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
Disassembler::disassemble(const std::vector<uint8_t> &input_buffer,
                          uint64_t base_address,
                          const std::vector<NamedSymbol> &static_symbols) {
  cs_insn *insn;
  const ssize_t count = cs_disasm(_handle, input_buffer.data(),
                                  input_buffer.size(), base_address, 0, &insn);
  if (count < 0)
    throw Status::disassembler__parse_failed;

  auto buffer_iterator = input_buffer.begin();
  std::vector<Disassembler::Line> result;
  for (int i = 0; i < count; i++) {
    const std::vector<uint16_t> opcodes(buffer_iterator,
                                        buffer_iterator + insn[i].size);
    const std::string instruction_operation = insn[i].mnemonic;
    std::string instruction_argument = insn[i].op_str;

    if (_is_resolvable_call_instruction(instruction_operation,
                                        instruction_argument))
      instruction_argument += _resolve_address(
          static_symbols, _hex_to_decimal(instruction_argument));

    result.emplace_back(opcodes, instruction_operation, instruction_argument,
                        insn[i].address, _is_jump(instruction_operation));
    buffer_iterator += insn[i].size;
  }
  cs_free(insn, count);
  return result;
}

bool Disassembler::_is_resolvable_call_instruction(
    const std::string &instruction_operation,
    const std::string &instruction_argument) {
  if (_is_call_instruction(instruction_operation))
    return _is_hex_number(instruction_argument);
  return false;
}

std::string
Disassembler::_resolve_address(const std::vector<NamedSymbol> &static_symbols,
                               uint64_t address) {
  // TODO: optimize, unordered map instead of vector
  for (const auto &symbol : static_symbols) {
    if (symbol.value == address)
      return " <" + symbol.name + ">";
  }
  return "";
}

uint64_t Disassembler::_hex_to_decimal(const std::string &number) {
  uint64_t result = 0;
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

bool Disassembler::_is_jump(std::string instruction) {
  std::vector<std::string_view> jump_values{"jmp", "je",  "jne", "jg",
                                            "jl",  "jge", "jle"};
  return std::find(jump_values.begin(), jump_values.end(), instruction) !=
         jump_values.end();
}
