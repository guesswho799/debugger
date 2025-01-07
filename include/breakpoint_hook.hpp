#pragma once

#include <sys/wait.h>
#include <sys/ptrace.h>
#include "status.hpp"

class BreakpointHook
{
public:
    BreakpointHook(uint64_t address, pid_t child_pid);
    ~BreakpointHook();

public:
    bool is_hit(int child_status) const;
    uint64_t get_address() const;

private:
    uint64_t _address;
    pid_t _child_pid;
    uint64_t _original_code;
};

