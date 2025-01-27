#pragma once

#include <capstone/capstone.h>
#include <string>
#include <vector>

class Disassembler {
public:
  struct Line {
    std::vector<int> opcodes;
    std::string instruction;
    std::string arguments;
    uint64_t address;
    bool is_jump;
  };

public:
  Disassembler();
  ~Disassembler();

  std::vector<Line> disassemble(uint8_t *input_buffer, size_t input_buffer_size,
                                int base_address);
  bool is_jump(std::string instruction);

private:
  csh _get_handler();

private:
  csh _handle;
};
