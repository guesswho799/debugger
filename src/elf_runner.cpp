#include "elf_runner.hpp"
#include "status.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <signal.h>
#include <iostream>


ElfRunner::ElfRunner(std::string file_name):
    _file_name(file_name),
    _child_pid(run(file_name)),
    _breakpoints(),
    _runtime_mapping(),
    _is_dead(false)
{}

ElfRunner::~ElfRunner()
{
    // TODO: adding this causes failure check why
    // kill(child_pid, SIGKILL);
    // wait(NULL);
}

void ElfRunner::reset()
{
    _runtime_mapping = {};
    _breakpoints.clear();
    kill(_child_pid, SIGKILL);
    wait(NULL);
    _child_pid = run(_file_name);
}

pid_t ElfRunner::run(std::string file_name)
{
    int child_status = 0;
    pid_t pid = fork();
    if (pid == -1)
    {
        throw CriticalException(Status::elf_runner__fork_failed);
    }
    
    // child
    if (pid == 0)
    {
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, STDOUT_FILENO);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        char* args[] = {const_cast<char*>(file_name.c_str()), NULL};
        execv(file_name.c_str(), args);
    }
    // debugger
    if (wait(&child_status) == -1)
    {
        throw CriticalException(Status::elf_runner__wait_failed);
    }
    if (WIFEXITED(child_status))
    {
        throw CriticalException(Status::elf_runner__child_died);
    }
   _is_dead = false;

    return pid;
}

bool ElfRunner::_non_blocking_get_to_address(uint64_t address, int child_status, std::vector<uint64_t> calls)
{
    // overwrite breakpoint code with interrupt
    if (_breakpoints.empty())
    {
        for (const auto& call_site: calls)
        {
            _breakpoints.emplace_back(call_site, _child_pid);
        }
	_breakpoints.emplace_back(address, _child_pid);
        if (ptrace(PTRACE_CONT, _child_pid, NULL, NULL) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__cont_failed); }
    }

    // run child until interupted
    auto is_hit = [&](BreakpointHook breakpoint){ return breakpoint.is_hit(child_status); };
    const auto iterator = std::find_if(_breakpoints.begin(), _breakpoints.end(), is_hit);
    if (iterator == _breakpoints.end()) return false;
    _breakpoints.erase(iterator);

    // resume regular flow
    struct user_regs_struct regs{};
    errno = 0;
    if (ptrace(PTRACE_GETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    regs.rip--;
    if (ptrace(PTRACE_SETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__poke_failed); }

    _step();
    _breakpoints.emplace_back(regs.rip, _child_pid);

    return true;
}

void ElfRunner::_update_is_dead(int child_status)
{
    if (_is_dead) return;

    _is_dead = WIFEXITED(child_status);
}

bool ElfRunner::is_dead() { return _is_dead; }

std::optional<ElfRunner::runtime_mapping> ElfRunner::run_function(uint64_t address, uint64_t size, std::vector<uint64_t> calls)
{
    int child_status = 0;
    int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
    if (status == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (status != 0) { _update_is_dead(child_status); }

    if (is_dead() or !_non_blocking_get_to_address(address, child_status, calls))
    {
	return _runtime_mapping;
    }

    struct user_regs_struct regs{};
    if (ptrace(PTRACE_GETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    while (regs.rip >= address and regs.rip <= address + size)
    {
	_step();
        if (ptrace(PTRACE_GETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    }
    errno = 0;
    if (ptrace(PTRACE_CONT, _child_pid, NULL, NULL) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__cont_failed); }

    return _runtime_mapping;
}

void ElfRunner::_step()
{
    struct user_regs_struct regs{};
    errno = 0;
    if (ptrace(PTRACE_GETREGS, _child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    _runtime_mapping[regs.rip]++;

    int child_status = 0;
    if (ptrace(PTRACE_SINGLESTEP, _child_pid, NULL, NULL) == -1) { throw CriticalException(Status::elf_runner__step_failed); }
    if (wait(&child_status) == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (WIFEXITED(child_status) or WIFSIGNALED(child_status)) { throw CriticalException(Status::elf_runner__child_finished); }
}

