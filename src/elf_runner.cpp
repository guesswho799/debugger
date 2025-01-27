#include "elf_runner.hpp"
#include "ptrace_utils.hpp"
#include "status.hpp"

#include <algorithm>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <unistd.h>

ElfRunner::ElfRunner(std::string file_name)
    : _file_name(file_name), _child_pid(run(file_name)), _breakpoints(),
      _runtime_regs(), _runtime_stacks(), _runtime_arguments(),
      _is_dead(false) {}

ElfRunner::~ElfRunner() {
  // TODO: adding this causes failure check why
  // kill(child_pid, SIGKILL);
  // wait(NULL);
}

void ElfRunner::reset() {
  _runtime_regs = {};
  _runtime_stacks = {};
  _runtime_arguments = {};
  _breakpoints.clear();
  kill(_child_pid, SIGKILL);
  wait(NULL);
  _child_pid = run(_file_name);
}

pid_t ElfRunner::run(std::string file_name) {
  const pid_t pid = fork();
  if (pid == -1)
    throw CriticalException(Status::elf_runner__fork_failed);

  // child
  if (pid == 0) {
    int dev_null = open("/dev/null", O_WRONLY);
    dup2(dev_null, STDOUT_FILENO);
    ptrace(PTRACE_TRACEME);
    char *args[] = {const_cast<char *>(file_name.c_str()), NULL};
    execv(file_name.c_str(), args);
  }
  _is_dead = false;

  return pid;
}

void ElfRunner::run_functions(const std::vector<NamedSymbol> &functions) {
  int child_status = 0;
  int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
  if (status == -1) {
    throw CriticalException(Status::elf_runner__wait_failed);
  }
  if (status != 0) {
    _update_is_dead(child_status);
  }
  if (is_dead() or !WIFSTOPPED(child_status))
    return;

  if (_breakpoints.empty()) {
    for (const auto &function : functions) {
      _breakpoints.emplace_back(function.value, _child_pid);
    }
    Ptrace::cont(_child_pid);
  } else {
    // check if breakpoint is hit
    const auto breakpoint =
        std::find_if(_breakpoints.begin(), _breakpoints.end(),
                     [&](const BreakpointHook &breakpoint) {
                       return breakpoint.is_hit(child_status);
                     });
    if (breakpoint == _breakpoints.end())
      return;

    // log function hit
    _log_function_arguments(functions, breakpoint->get_address());

    // resume child regular flow
    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    regs.rip--;
    Ptrace::set_regs(_child_pid, regs);
    breakpoint->unhook();
    _log_step();
    breakpoint->hook();
    Ptrace::cont(_child_pid);
  }
}

void ElfRunner::run_function(const NamedSymbol &function,
                             const std::vector<Address> &calls) {
  int child_status = 0;
  int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
  if (status == -1) {
    throw CriticalException(Status::elf_runner__wait_failed);
  }
  if (status != 0) {
    _update_is_dead(child_status);
  }
  if (is_dead() or !WIFSTOPPED(child_status))
    return;

  if (_breakpoints.empty()) {
    _breakpoints.emplace_back(function.value, _child_pid);
    for (const auto &call : calls) {
      _breakpoints.emplace_back(call, _child_pid);
    }
    Ptrace::cont(_child_pid);
  } else {
    // check if breakpoint is hit
    const auto breakpoint =
        std::find_if(_breakpoints.begin(), _breakpoints.end(),
                     [&](const BreakpointHook &breakpoint) {
                       return breakpoint.is_hit(child_status);
                     });
    if (breakpoint == _breakpoints.end())
      return;

    // resume child regular flow
    struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
    regs.rip--;
    Ptrace::set_regs(_child_pid, regs);
    breakpoint->unhook();
    while (regs.rip >= function.value and
           regs.rip <= function.value + function.size) {
      _log_step();
      regs = Ptrace::get_regs(_child_pid);
    }
    breakpoint->hook();
    Ptrace::cont(_child_pid);
  }
}

void ElfRunner::_log_step() {
  struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
  errno = 0;
  _runtime_regs.push_back(regs);

  const auto base_stack = regs.rbp;
  std::array<StackElement, stack_size> stack{};
  if (base_stack != 0)
    for (int i = 0; i < stack_size; i++)
      stack[i] = Ptrace::get_memory(_child_pid,
                                    base_stack - (i * sizeof(StackElement)));
  _runtime_stacks.emplace_back(base_stack, stack);

  int child_status = 0;
  Ptrace::single_step(_child_pid);
  if (wait(&child_status) == -1) {
    throw CriticalException(Status::elf_runner__wait_failed);
  }
  if (WIFEXITED(child_status) or WIFSIGNALED(child_status)) {
    throw CriticalException(Status::elf_runner__child_finished);
  }
}

void ElfRunner::_log_function_arguments(
    const std::vector<NamedSymbol> &functions, Address function_address) {
  const auto get_function_by_address = [&](NamedSymbol function) {
    return function.value == function_address;
  };
  const auto iterator =
      std::find_if(functions.begin(), functions.end(), get_function_by_address);
  if (iterator == functions.end())
    throw CriticalException(Status::elf_runner__unable_to_find_function);

  const auto regs = Ptrace::get_regs(_child_pid);
  _runtime_arguments[iterator->name] = {regs.rdi, regs.rsi, regs.rdx};
}

void ElfRunner::_update_is_dead(int child_status) {
  if (_is_dead)
    return;

  _is_dead = WIFEXITED(child_status);
}

bool ElfRunner::is_dead() const { return _is_dead; }

ElfRunner::RuntimeArguments ElfRunner::get_runtime_arguments() const {
  return _runtime_arguments;
}

ElfRunner::RuntimeRegs ElfRunner::get_runtime_regs() const {
  return _runtime_regs;
}

ElfRunner::RuntimeStacks ElfRunner::get_runtime_stacks() const {
  return _runtime_stacks;
}
