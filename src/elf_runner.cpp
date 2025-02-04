#include "elf_runner.hpp"
#include "ptrace_utils.hpp"
#include "status.hpp"

#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <regex>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <unistd.h>

ElfRunner::ElfRunner(std::string file_name)
    : _file_name(file_name), _child_pid(_run(file_name)), _breakpoints(),
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
  _child_pid = _run(_file_name);
}

pid_t ElfRunner::_run(std::string file_name) {
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

uint64_t ElfRunner::get_base_address(bool is_position_independent) {
  if (!is_position_independent or _is_dead)
    return 0;

  const std::regex pattern("([0-9a-f]+)-.*");
  std::ifstream mapping_file(std::format("/proc/{}/maps", _child_pid));
  std::string line;
  std::smatch match;
  std::getline(mapping_file, line);
  if (std::regex_match(line, match, pattern)) {
    return _hex_to_int(match[1].str());
  }

  _is_dead = true;
  return 0;
}

void ElfRunner::run_functions(const std::vector<NamedSymbol> &functions, uint64_t base_address) {
  const int child_status = _get_child_status();
  if (_check_child_status(child_status))
    return;

  if (_breakpoints.empty()) {
    for (const auto &function : functions) {
      _breakpoints.emplace_back(function.value, _child_pid);
    }
    Ptrace::cont(_child_pid);
    return;
  }

  const auto breakpoint = std::find_if(_breakpoints.begin(), _breakpoints.end(),
                                       [&](const BreakpointHook &breakpoint) {
                                         return breakpoint.is_hit(child_status);
                                       });
  if (breakpoint == _breakpoints.end())
    return;

  _log_function_arguments(functions, breakpoint->get_address());

  // resume child regular flow
  struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
  regs.rip--;
  Ptrace::set_regs(_child_pid, regs);
  breakpoint->unhook();
  _log_step(base_address);
  breakpoint->hook();
  Ptrace::cont(_child_pid);
}

void ElfRunner::run_function(const NamedSymbol &function,
                             const std::vector<Address> &calls, uint64_t base_address) {
  const int child_status = _get_child_status();
  if (_check_child_status(child_status))
    return;

  if (_breakpoints.empty()) {
    _breakpoints.emplace_back(function.value, _child_pid);
    for (const auto &call : calls) {
      _breakpoints.emplace_back(call, _child_pid);
    }
    Ptrace::cont(_child_pid);
    return;
  }

  const auto breakpoint = std::find_if(_breakpoints.begin(), _breakpoints.end(),
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
    _log_step(base_address);
    regs = Ptrace::get_regs(_child_pid);
  }
  breakpoint->hook();
  Ptrace::cont(_child_pid);
}

void ElfRunner::_log_step(uint64_t base_address) {
  struct user_regs_struct regs = Ptrace::get_regs(_child_pid);
  regs.rip -= base_address;
  _runtime_regs.push_back(regs);

  const auto base_stack = regs.rbp;
  std::array<StackElement, stack_size> stack{};
  if (base_stack != 0)
    for (int i = 0; i < stack_size; i++)
      stack[i] = static_cast<StackElement>(Ptrace::get_memory(
          _child_pid, base_stack - (i * sizeof(StackElement))));
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

int ElfRunner::_get_child_status() {
  int child_status = 0;
  int status = waitpid(_child_pid, &child_status, WNOHANG | WUNTRACED);
  if (status == -1) {
    throw CriticalException(Status::elf_runner__wait_failed);
  }
  if (status != 0) {
    _update_is_dead(child_status);
  }
  return child_status;
}

bool ElfRunner::_check_child_status(int child_status) {
  return is_dead() or !WIFSTOPPED(child_status);
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

pid_t ElfRunner::get_pid() const { return _child_pid; }

uint64_t ElfRunner::_hex_to_int(const std::string &s) {
  uint64_t result;
  std::stringstream ss;
  ss << std::hex << s;
  ss >> result;
  return result;
}
