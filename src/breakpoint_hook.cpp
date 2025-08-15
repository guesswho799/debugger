#include "breakpoint_hook.hpp"
#include "ptrace_utils.hpp"
#include <sys/reg.h>
#include <sys/user.h>
#include <utility>

BreakpointHook::BreakpointHook(uint64_t address, pid_t child_pid)
    : _address(address), _child_pid(child_pid),
      _original_code(Ptrace::get_memory(child_pid, address)) {
  hook();
}

BreakpointHook::~BreakpointHook() noexcept {
  if (_child_pid != 0)
    try {
      unhook();
    } catch (...) {
    }
}

BreakpointHook::BreakpointHook(BreakpointHook &&other) noexcept
    : _address(std::exchange(other._address, 0)),
      _child_pid(std::exchange(other._child_pid, 0)),
      _original_code(std::exchange(other._original_code, 0)) {}

BreakpointHook &BreakpointHook::operator=(BreakpointHook &&other) noexcept {
  BreakpointHook temp(std::move(other));
  _address = temp._address;
  _child_pid = temp._child_pid;
  _original_code = temp._original_code;
  return *this;
}

bool BreakpointHook::is_hit(int child_status) const {
  if (!WIFSTOPPED(child_status))
    return false;
  return Ptrace::get_regs(_child_pid).rip == _address + 1;
}

bool BreakpointHook::is_hooked() const {
  return Ptrace::get_memory(_child_pid, _address) != _original_code;
}

void BreakpointHook::hook() {
  const uint64_t new_breakpoint_code =
      (_original_code & (~(uint64_t)0xFF)) | 0xCC;
  Ptrace::set_memory(_child_pid, _address, new_breakpoint_code);
}

void BreakpointHook::unhook() {
  Ptrace::set_memory(_child_pid, _address, _original_code);
}

uint64_t BreakpointHook::get_address() const { return _address; }
