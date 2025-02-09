#pragma once

#include "elf_header.hpp"
#include <cstdint>
#include <dis-asm.h>
#include <string>
#include <vector>

namespace GlobalData {
struct stream_state {
  char *insn_buffer;
  bool reenter;
};
constexpr int MAX_DISASSEMBLE_LINE = 120;
static char buffer[MAX_DISASSEMBLE_LINE]{};
static int current_index = 0;

inline static int _dis_fprintf(void *stream, const char *fmt, ...) {
  stream_state *ss = (stream_state *)stream;

  va_list arg;
  va_start(arg, fmt);
  if (!ss->reenter) {
    if (vasprintf(&ss->insn_buffer, fmt, arg) == -1)
      throw 1;
    ss->reenter = true;
  } else {
    char *tmp;
    if (vasprintf(&tmp, fmt, arg) == -1)
      throw 2;

    char *tmp2;
    if (asprintf(&tmp2, "%s%s", ss->insn_buffer, tmp) == -1)
      throw 3;
    free(ss->insn_buffer);
    free(tmp);
    ss->insn_buffer = tmp2;
  }
  va_end(arg);

  return 0;
}

inline static int _fprintf_styled(void *, enum disassembler_style,
                                  const char *fmt, ...) {
  va_list args;
  int size;

  va_start(args, fmt);
  size = vsprintf(buffer + current_index, fmt, args);
  current_index += size;
  va_end(args);

  return size;
}
} // namespace GlobalData

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

  std::vector<Line>
  disassemble(const std::vector<uint8_t> &input_buffer, uint64_t base_address,
              const std::vector<NamedSymbol> &static_symbols = {},
              const std::vector<NamedSymbol> &dynamic_symbols = {},
              const std::vector<ElfString> &strings = {});
  static int64_t get_address(const std::string &instruction_argument);

private:
  static bool
  _is_resolvable_call_instruction(const std::string &instruction_operation,
                                  const std::string &instruction_argument);
  static std::string
  _resolve_address(const std::vector<NamedSymbol> &static_symbols,
                   const std::vector<NamedSymbol> &dynamic_symbols,
                   const std::vector<ElfString> &strings, uint64_t address);
  static std::string
  _resolve_symbol(const std::vector<NamedSymbol> &static_symbols,
                  const std::vector<NamedSymbol> &dynamic_symbols,
                  uint64_t address);
  static std::string _resolve_string(const std::vector<ElfString> &strings,
                                     uint64_t address);
  static int64_t _hex_to_decimal(const std::string &number);
  static bool _is_hex_number(const std::string &number);
  static bool _is_call_instruction(const std::string &s);
  static bool _is_load_instruction(const std::string &s);
  static bool _is_jump(std::string instruction);

private:
};
