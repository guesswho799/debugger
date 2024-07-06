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
    ptrace(PTRACE_CONT, child_pid);
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

void ElfRunner::get_to_address(uint64_t address)
{
    int child_status = 0;
    struct user_regs_struct regs{};

    ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    while (regs.rip != address)
    {
        ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        wait(&child_status);
        if (WIFEXITED(child_status))
        {
            throw CriticalException(Status::elf_runner__child_finished);
        }

        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    }
}

std::map<int, int> ElfRunner::run_function(uint64_t address, uint64_t size)
{
    std::map<int, int> address_counter{};
    get_to_address(address);

    int child_status = 0;
    struct user_regs_struct regs{};

    ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    while (regs.rip >= address and regs.rip <= address + size)
    {
        address_counter[regs.rip]++;

        ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
        wait(&child_status);
        if (WIFEXITED(child_status) or WIFSIGNALED(child_status))
        {
            std::cout << "program exited\n";
            break;
        }
        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    }

    return address_counter;
}
