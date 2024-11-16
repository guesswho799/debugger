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
    child_pid(run(file_name)),
    _breakpoint()
{}

ElfRunner::~ElfRunner()
{
    // TODO: adding this causes failure check why
    // kill(child_pid, SIGKILL);
    // wait(NULL);
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

    return pid;
}

bool ElfRunner::get_to_address(uint64_t address)
{
    // overwrite breakpoint code with interrupt
    if (!_breakpoint.has_value()) { _breakpoint.emplace(address, child_pid); }

    // run child until interupted
    int child_status = 0;
    if (waitpid(child_pid, &child_status, WNOHANG | WUNTRACED) == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (!WIFSTOPPED(child_status) or WSTOPSIG(child_status) != SIGTRAP) return false;

    // resume regular flow
    struct user_regs_struct regs{};
    errno = 0;
    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    regs.rip--;
    if (ptrace(PTRACE_SETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__poke_failed); }
    _breakpoint.reset();

    return true;
}

std::optional<ElfRunner::runtime_mapping> ElfRunner::run_function(uint64_t address, uint64_t size)
{
    if (!get_to_address(address))
    {
	return {};
    }

    struct user_regs_struct regs{};
    runtime_mapping address_counter{};
    int child_status = 0;

    errno = 0;
    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    while (regs.rip >= address and regs.rip <= address + size)
    {
        address_counter[regs.rip]++;

	if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) == -1) { throw CriticalException(Status::elf_runner__step_failed); }
	if (wait(&child_status) == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
	if (WIFEXITED(child_status) or WIFSIGNALED(child_status)) { throw CriticalException(Status::elf_runner__child_finished); }

        errno = 0;
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    }

    return address_counter;
}

