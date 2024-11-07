#include "elf_runner.hpp"
#include "status.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <iostream>


ElfRunner::ElfRunner(std::string file_name):
    child_pid(run(file_name))
{}

ElfRunner::~ElfRunner()
{
    ptrace(PTRACE_CONT, child_pid, NULL, NULL);
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
    int child_status = 0;
    struct user_regs_struct regs{};

    errno = 0;
    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    for (int counter = 0; counter < 10000 and regs.rip != address; counter++)
    {
        if (ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL) == -1) { throw CriticalException(Status::elf_runner__step_failed); }
        if (wait(&child_status) == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
        if (WIFEXITED(child_status) or WIFSIGNALED(child_status)) { throw CriticalException(Status::elf_runner__child_finished); }
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
    }

    return regs.rip == address;
}

std::variant<ElfRunner::runtime_mapping, ElfRunner::current_address> ElfRunner::run_function(uint64_t address, uint64_t size)
{
    struct user_regs_struct regs{};
    if (!get_to_address(address))
    {
	errno = 0;
	if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1 or errno != 0) { throw CriticalException(Status::elf_runner__peek_failed); }
	return static_cast<current_address>(regs.rip);
    }

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
