#pragma once

#include "elf_header.hpp"
#include <capstone/capstone.h>
#include <cstdint>
#include <string>
#include <vector>

class Disassembler {
public:
  struct Line {
    std::vector<unsigned int> opcodes;
    std::string instruction;
    std::string arguments;
    uint64_t address;
    bool is_jump;
  };

public:
  Disassembler();
  ~Disassembler();

  std::vector<Line> disassemble(uint8_t *input_buffer, size_t input_buffer_size,
                                uint64_t base_address,
                                const std::vector<NamedSymbol> &static_symbols);

private:
  static uint64_t hex_to_decimal(const std::string &number);
  static bool is_hex_number(const std::string &number);
  static bool
  is_resolvable_call_instruction(const std::string &instruction_operation,
                                 const std::string &instruction_argument);
  static std::string
  resolve_address(const std::vector<NamedSymbol> &static_symbols,
                  uint64_t address);
  static bool is_jump(std::string instruction);

private:
  csh _get_handler();

private:
  csh _handle;
};
