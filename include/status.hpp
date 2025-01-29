#pragma once
#include <exception>
#include <iostream>
#include <string>

#define BEST_EFFORT(code)                                                      \
  try {                                                                        \
    code;                                                                      \
  } catch (...) {                                                              \
  }

enum class Status : int {
  success = 0,
  elf_header__open_failed,
  elf_header__function_not_found,
  elf_header__section_not_found,
  elf_runner__fork_failed,
  elf_runner__wait_failed,
  elf_runner__child_died,
  elf_runner__child_finished,
  elf_runner__step_failed,
  ptrace__peek_regs_failed,
  ptrace__poke_regs_failed,
  ptrace__peek_code_failed,
  ptrace__poke_code_failed,
  ptrace__cont_failed,
  elf_runner__unable_to_find_function,
  disassembler__open_failed,
  disassembler__parse_failed,
};

class CriticalException : public std::exception {
public:
  CriticalException(Status msg) : status(msg) {}

  const char *what() const throw() {
    std::string output = std::to_string(static_cast<int>(status));
    std::cout << static_cast<int>(status);
    return "";
  }

private:
  Status status;
};

