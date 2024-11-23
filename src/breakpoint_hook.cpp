#include "breakpoint_hook.hpp"
#include <sys/reg.h>
#include <sys/user.h>


BreakpointHook::BreakpointHook(uint64_t address, pid_t child_pid):
        _address(address),
        _child_pid(child_pid),
        _original_code(_get_function_code(address, child_pid))
{
    const uint64_t new_breakpoint_code = (_original_code & (~(uint64_t)0xFF)) | 0xCC;
    errno = 0;
    if (ptrace(PTRACE_POKETEXT, child_pid, address, new_breakpoint_code) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__poke_failed); }
}

BreakpointHook::~BreakpointHook()
{
    // best effort not checking return value
    ptrace(PTRACE_POKETEXT, _child_pid, _address, _original_code);
}

bool BreakpointHook::is_hit(int child_status) const
{
    if (WIFSTOPPED(child_status))
    {
        struct user_regs_struct regs{};
        errno = 0;
        if (ptrace(PTRACE_GETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
	return regs.rip-1 == _address;
    }
    return false;
}

uint64_t BreakpointHook::_get_function_code(uint64_t address, pid_t child_pid)
{
    errno = 0;
    const uint64_t function_code = ptrace(PTRACE_PEEKTEXT, child_pid, address, NULL);
    if (function_code == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    return function_code;  
}

