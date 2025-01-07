#include "elf_runner.hpp"
#include "ptrace_utils.hpp"
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
    _runtime_arguments(),
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

bool ElfRunner::_non_blocking_get_to_breakpoints(const std::vector<NamedSymbol>& functions, int child_status)
{
    // overwrite breakpoint code with interrupt
    if (_breakpoints.empty() and _runtime_arguments.empty())
    {
        for (const auto& function: functions)
        {
            _breakpoints.emplace_back(function.value, _child_pid);
        }
	Ptrace::cont(_child_pid);
    }

    // check if child is interupted
    const auto is_hit = [&](BreakpointHook breakpoint){ return breakpoint.is_hit(child_status); };
    const auto iterator = std::find_if(_breakpoints.begin(), _breakpoints.end(), is_hit);
    if (iterator == _breakpoints.end()) return false;
    _log_function_arguments(functions, iterator->get_address());
    _breakpoints.erase(iterator);

    // resume child regular flow
    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    regs.rip--;
    Ptrace::set_regs(_child_pid, regs);

    Ptrace::cont(_child_pid);

    return true;
}

std::optional<ElfRunner::runtime_mapping> ElfRunner::run_function(uint64_t address, uint64_t size, std::vector<uint64_t> calls)
{
    int child_status = 0;
    int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
    if (status == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (status != 0) { _update_is_dead(child_status); }

  //if (is_dead() or !_non_blocking_get_to_address(address, child_status, calls))
  //{
  //    return _runtime_mapping;
  //}

    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    while (regs.rip >= address and regs.rip <= address + size)
    {
	_log_step();
        regs = Ptrace::get_regs(_child_pid);
    }
    Ptrace::cont(_child_pid);

    return _runtime_mapping;
}

std::optional<ElfRunner::runtime_arguments> ElfRunner::run_functions(const std::vector<NamedSymbol>& functions)
{
    int child_status = 0;
    int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
    if (status == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (status != 0) { _update_is_dead(child_status); }

    if (is_dead() or !_non_blocking_get_to_breakpoints(functions, child_status))
    {
	return _runtime_arguments;
    }

    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    Ptrace::cont(_child_pid);

    return _runtime_arguments;
}

void ElfRunner::_log_step()
{
    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    errno = 0;
    _runtime_mapping[regs.rip]++;

    int child_status = 0;
    Ptrace::single_step(_child_pid);
    if (wait(&child_status) == -1) { throw CriticalException(Status::elf_runner__wait_failed); }
    if (WIFEXITED(child_status) or WIFSIGNALED(child_status)) { throw CriticalException(Status::elf_runner__child_finished); }
}

void ElfRunner::_log_function_arguments(const std::vector<NamedSymbol>& functions, address_t function_address)
{
    const auto get_function_by_address = [&](NamedSymbol function){ return function.value == function_address; };
    const auto iterator = std::find_if(functions.begin(), functions.end(), get_function_by_address);
    if (iterator == functions.end()) throw CriticalException(Status::elf_runner__unable_to_find_function);

    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    _runtime_arguments[iterator->name] = regs.rax;
}

void ElfRunner::_update_is_dead(int child_status)
{
    if (_is_dead) return;

    _is_dead = WIFEXITED(child_status);
}

bool ElfRunner::is_dead() { return _is_dead; }


std::optional<ElfRunner::runtime_arguments> ElfRunner::get_runtime_arguments() const
{
    return _runtime_arguments;
}

