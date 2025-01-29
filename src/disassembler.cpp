#include "disassembler.hpp"
#include "status.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string.h>

Disassembler::Disassembler() : _handle(_get_handler()) {
  cs_option(_handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
}

Disassembler::~Disassembler() { cs_close(&_handle); }

csh Disassembler::_get_handler() {
  csh handle;
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
    throw Status::disassembler__open_failed;
  return handle;
}

std::vector<Disassembler::Line>
Disassembler::disassemble(uint8_t *input_buffer, size_t input_buffer_size,
                          uint64_t base_address,
                          const std::vector<NamedSymbol> &static_symbols) {
  cs_insn *insn;
  std::vector<Disassembler::Line> result;
  const ssize_t count = cs_disasm(_handle, input_buffer, input_buffer_size,
                                  base_address, 0, &insn);
  if (count < 0)
    throw Status::disassembler__parse_failed;
  int pc = 0;
  for (int i = 0; i < count; i++) {
    std::vector<unsigned int> opcodes;
    opcodes.insert(opcodes.end(), input_buffer + pc,
                   input_buffer + pc + insn[i].size);

    const std::string instruction_operation = insn[i].mnemonic;
    std::string instruction_argument = insn[i].op_str;
    if (is_resolvable_call_instruction(instruction_operation,
                                       instruction_argument)) {
      instruction_argument +=
          resolve_address(static_symbols, hex_to_decimal(instruction_argument));
    }
    result.emplace_back(opcodes, insn[i].mnemonic, instruction_argument,
                        insn[i].address, is_jump(insn[i].mnemonic));
    pc += insn[i].size;
  }
  cs_free(insn, count);
  return result;
}

uint64_t Disassembler::hex_to_decimal(const std::string &number) {
  uint64_t result = 0;
  std::stringstream ss;
  ss << std::hex << number;
  ss >> result;
  return result;
}

bool Disassembler::is_hex_number(const std::string &s) {
  if (!s.starts_with("0x"))
    return false;

  constexpr int skip_prefix = 2;
  std::string::const_iterator it = s.begin() + skip_prefix;
  while (it != s.end() and std::isxdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

bool Disassembler::is_resolvable_call_instruction(
    const std::string &instruction_operation,
    const std::string &instruction_argument) {
  if (0 == strncmp(instruction_operation.c_str(), "call", 4))
    return is_hex_number(instruction_argument);
  return false;
}

std::string
Disassembler::resolve_address(const std::vector<NamedSymbol> &static_symbols,
                              uint64_t address) {
  // TODO: optimize, unordered map instead of vector
  for (const auto &symbol : static_symbols) {
    if (symbol.value == address)
      return " <" + symbol.name + ">";
  }
  return "";
}

bool Disassembler::is_jump(std::string instruction) {
  std::vector<std::string_view> jump_values{"jmp", "je",  "jne", "jg",
                                            "jl",  "jge", "jle"};
  return std::find(jump_values.begin(), jump_values.end(), instruction) !=
         jump_values.end();
}
