#pragma once

#include "elf_header.hpp"
#include <capstone/capstone.h>
#include <cstdint>
#include <string>
#include <vector>

class Disassembler {
public:
  struct Line {
    std::vector<uint16_t> opcodes;
    std::string instruction;
    std::string arguments;
    uint64_t address;
    bool is_jump;
  };

public:
  Disassembler();
  ~Disassembler();

  std::vector<Line> disassemble(const std::vector<uint8_t> &input_buffer,
                                uint64_t base_address,
                                const std::vector<NamedSymbol> &static_symbols);

private:
  static bool _is_resolvable_call_instruction(const std::string &instruction_operation,
                                 const std::string &instruction_argument);
  static std::string
  _resolve_address(const std::vector<NamedSymbol> &static_symbols,
                  uint64_t address);
  static uint64_t _hex_to_decimal(const std::string &number);
  static bool _is_hex_number(const std::string &number);
  static bool _is_call_instruction(const std::string &s);
  static bool _is_jump(std::string instruction);

private:
  csh _get_handler();

private:
  csh _handle;
};
