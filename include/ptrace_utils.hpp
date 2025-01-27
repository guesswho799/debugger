#pragma once

#include <cerrno>
#include <cstdint>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/user.h>
#include "status.hpp"

namespace Ptrace {
inline user_regs_struct get_regs(pid_t pid) {
  struct user_regs_struct regs{};
  errno = 0;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1 or errno != 0) {
    throw CriticalException(Status::ptrace__peek_regs_failed);
  }
  return regs;
}

inline void set_regs(pid_t pid, user_regs_struct regs) {
  errno = 0;
  if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1 or errno != 0) {
    throw CriticalException(Status::ptrace__poke_regs_failed);
  }
}

inline uint64_t get_memory(pid_t pid, uint64_t address) {
  errno = 0;
  const uint64_t code = ptrace(PTRACE_PEEKTEXT, pid, address, NULL);
  if (errno != 0) {
    throw CriticalException(Status::ptrace__peek_code_failed);
  }
  return code;
}

inline void set_memory(pid_t pid, uint64_t address, uint64_t code) {
  errno = 0;
  if (ptrace(PTRACE_POKETEXT, pid, address, code) == -1 or errno != 0) {
    throw CriticalException(Status::ptrace__poke_code_failed);
  }
}

inline void single_step(pid_t pid) {
  errno = 0;
  if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1 or errno != 0) {
    throw CriticalException(Status::elf_runner__step_failed);
  }
}

inline void cont(pid_t pid) {
  errno = 0;
  if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1 or errno != 0) {
    throw CriticalException(Status::ptrace__cont_failed);
  }
}
} // namespace Ptrace
