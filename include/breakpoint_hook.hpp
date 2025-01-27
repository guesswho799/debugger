#pragma once

#include <cstdint>
#include <sys/ptrace.h>
#include <sys/wait.h>

class BreakpointHook {
public:
  BreakpointHook(uint64_t address, pid_t child_pid);
  ~BreakpointHook() noexcept;
  BreakpointHook(const BreakpointHook &) = delete;
  BreakpointHook &operator=(const BreakpointHook &) = delete;
  BreakpointHook(BreakpointHook &&) noexcept;
  BreakpointHook &operator=(BreakpointHook &&) noexcept;

public:
  bool is_hit(int child_status) const;
  bool is_hooked() const;
  void hook();
  void unhook();

  uint64_t get_address() const;

public:
  uint64_t _address;
  pid_t _child_pid;
  uint64_t _original_code;
};
