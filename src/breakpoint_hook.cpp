#include "breakpoint_hook.hpp"
#include "ptrace_utils.hpp"
#include <sys/reg.h>
#include <sys/user.h>


BreakpointHook::BreakpointHook(uint64_t address, pid_t child_pid):
        _address(address),
        _child_pid(child_pid),
        _original_code(Ptrace::get_code(child_pid, address))
{
    const uint64_t new_breakpoint_code = (_original_code & (~(uint64_t)0xFF)) | 0xCC;
    Ptrace::set_code(child_pid, address, new_breakpoint_code);
}

BreakpointHook::~BreakpointHook()
{
    BEST_EFFORT(Ptrace::set_code(_child_pid, _address, _original_code));
}

bool BreakpointHook::is_hit(int child_status) const
{
    if (WIFSTOPPED(child_status))
    {
        struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
	return regs.rip-1 == _address;
    }
    return false;
}

uint64_t BreakpointHook::get_address() const
{
    return _address;
}

