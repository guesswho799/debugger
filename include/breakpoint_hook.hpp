#pragma once

#include <sys/ptrace.h>
#include "status.hpp"

class BreakpointHook
{
public:
    BreakpointHook(uint64_t address, pid_t child_pid):
	    _address(address),
	    _child_pid(child_pid),
	    _original_code(get_function_code(address, child_pid))
    {
        errno = 0;
        const uint64_t new_breakpoint_code = _original_code & (~(uint64_t)0xFF) | 0xCC;
        if (ptrace(PTRACE_POKETEXT, child_pid, address, new_breakpoint_code) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__poke_failed); }
        if (ptrace(PTRACE_CONT, child_pid, NULL, NULL) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__cont_failed); }
    }

    ~BreakpointHook()
    {
	// best effort not checking return value
        ptrace(PTRACE_POKETEXT, _child_pid, _address, _original_code);
    }

private:
    uint64_t get_function_code(uint64_t address, pid_t child_pid)
    {
        errno = 0;
        const uint64_t function_code = ptrace(PTRACE_PEEKTEXT, child_pid, address, NULL);
        if (function_code == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
	return function_code;  
    }
    uint64_t _address;
    pid_t _child_pid;
    uint64_t _original_code;
};

