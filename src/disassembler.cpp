#include "disassembler.hpp"
#include "status.hpp"

#include <algorithm>
#include <bfd.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dis-asm.h>
#include <elf.h>
#include <regex>
#include <sstream>
#include <string.h>
#include <string>

Disassembler::Disassembler() {}

Disassembler::~Disassembler() {}

// csh Disassembler::_get_handler() {
//   csh handle;
//   if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
//     throw Status::disassembler__open_failed;
//   return handle;
// }

std::vector<Disassembler::Line>
Disassembler::disassemble(const std::vector<uint8_t> &input_buffer,
                          uint64_t base_address,
                          const std::vector<NamedSymbol> &static_symbols,
                          const std::vector<NamedSymbol> &dynamic_symbols,
                          const std::vector<ElfString> &strings) {
  if (static_symbols.size() or dynamic_symbols.size() or strings.size())
    std::cout << " ";
  GlobalData::stream_state ss{};
  disassemble_info info{};
  init_disassemble_info(&info, &ss, GlobalData::_dis_fprintf,
                        GlobalData::_fprintf_styled);
  info.arch = bfd_arch_i386;
  info.mach = bfd_mach_x86_64;
  info.disassembler_options = "intel";
  info.buffer = const_cast<unsigned char *>(input_buffer.data());
  info.buffer_vma = 0;
  info.buffer_length = input_buffer.size();
  disassemble_init_for_target(&info);
  disassembler_ftype disassm =
      disassembler(bfd_arch_i386, false, bfd_mach_x86_64, NULL);
  auto opcode_iterator = input_buffer.begin();
  std::vector<Disassembler::Line> result;

  while (opcode_iterator != input_buffer.end()) {
    const uint64_t index = opcode_iterator - input_buffer.begin();
    const size_t instruction_size = disassm(index, &info);

    std::vector<uint16_t> opcodes;
    opcodes.insert(opcodes.end(), opcode_iterator,
                   opcode_iterator + instruction_size);

    result.emplace_back(opcodes, GlobalData::buffer, "", base_address + index,
                        _is_jump(GlobalData::buffer));
    GlobalData::current_index = 0;
    memset(GlobalData::buffer, 0, GlobalData::MAX_DISASSEMBLE_LINE);
    opcode_iterator += instruction_size;
  }

  return result;
}
// const std::vector<uint16_t> opcodes(buffer_iterator,
//                                     buffer_iterator + insn[i].size);
// const std::string instruction_operation = insn[i].mnemonic;
// const uint64_t instruction_address = insn[i].address;
// const uint16_t instruction_size = insn[i].size;
// std::string instruction_argument = insn[i].op_str;

// if (_is_resolvable_call_instruction(instruction_operation,
//                                     instruction_argument))
//   instruction_argument +=
//       _resolve_symbol(static_symbols, dynamic_symbols,
//                       _hex_to_decimal(instruction_argument));
// if (_is_load_instruction(instruction_operation))
//   instruction_argument +=
//       _resolve_address(static_symbols, dynamic_symbols, strings,
//                        instruction_address + instruction_size +
//                            get_address(instruction_argument));

bool Disassembler::_is_resolvable_call_instruction(
    const std::string &instruction_operation,
    const std::string &instruction_argument) {
  if (_is_call_instruction(instruction_operation))
    return _is_hex_number(instruction_argument);
  return false;
}

std::string
Disassembler::_resolve_address(const std::vector<NamedSymbol> &static_symbols,
                               const std::vector<NamedSymbol> &dynamic_symbols,
                               const std::vector<ElfString> &strings,
                               uint64_t address) {
  const std::string symbol =
      _resolve_symbol(static_symbols, dynamic_symbols, address);
  if (!symbol.empty())
    return symbol;

  const std::string s = _resolve_string(strings, address);
  if (!s.empty())
    return s;

  return " " + std::to_string(address);
}

std::string
Disassembler::_resolve_symbol(const std::vector<NamedSymbol> &static_symbols,
                              const std::vector<NamedSymbol> &dynamic_symbols,
                              uint64_t address) {
  // TODO: optimize, unordered map instead of vector
  for (const auto &symbol : static_symbols) {
    if (symbol.value == address)
      return " <" + symbol.name + ">";
  }

  for (const auto &symbol : dynamic_symbols) {
    if (symbol.value == address)
      return " <" + symbol.name + "/external>";
  }

  return "";
}

int64_t Disassembler::get_address(const std::string &instruction_argument) {
  const std::regex pattern(".*\\[rip ([\\+-]) 0x([0-9a-f]+)\\]");
  std::smatch match;
  if (std::regex_match(instruction_argument, match, pattern)) {
    int64_t address = _hex_to_decimal(match[2].str());
    if (0 == strncmp(match[1].str().c_str(), "-", 1)) {
      address *= -1;
    }
    return address;
  }
  return 0;
}

std::string Disassembler::_resolve_string(const std::vector<ElfString> &strings,
                                          uint64_t address) {
  constexpr size_t max_string_size = 15;
  for (const auto &s : strings) {
    if (s.address == address) {
      std::string result = s.value;
      if (result.size() > max_string_size) {
        result.resize(max_string_size - 3);
        result += "...";
      }
      return " \"" + result + "\"";
    }
  }
  return "";
}

int64_t Disassembler::_hex_to_decimal(const std::string &number) {
  int64_t result = 0;
  std::stringstream ss;
  ss << std::hex << number;
  ss >> result;
  return result;
}

bool Disassembler::_is_hex_number(const std::string &s) {
  if (!s.starts_with("0x"))
    return false;

  constexpr int skip_prefix = 2;
  std::string::const_iterator it = s.begin() + skip_prefix;
  while (it != s.end() and std::isxdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

bool Disassembler::_is_call_instruction(const std::string &s) {
  return 0 == strncmp(s.c_str(), "call", 4);
}

bool Disassembler::_is_load_instruction(const std::string &s) {
  return 0 == strncmp(s.c_str(), "lea", 3);
}

bool Disassembler::_is_jump(std::string instruction) {
  std::vector<std::string_view> jump_values{"jmp", "je",  "jne", "jg",
                                            "jl",  "jge", "jle"};
  return std::find(jump_values.begin(), jump_values.end(), instruction) !=
         jump_values.end();
}
