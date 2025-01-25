#pragma once

#include "breakpoint_hook.hpp"
#include "elf_header.hpp"

#include <sys/user.h>
#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <vector>
#include <array>


class ElfRunner
{
public:
    using Address          = uint64_t;
    using RuntimeRegs      = std::vector<struct user_regs_struct>;
    using RuntimeArguments = std::map<std::string, std::tuple<int64_t, int64_t, int64_t>>;

    static constexpr int stack_size = 10;
    using StackElement              = uint32_t;
    using RuntimeStack              = std::pair<Address, std::array<StackElement, stack_size>>;
    using RuntimeStacks             = std::vector<RuntimeStack>;

public:
    explicit ElfRunner(std::string file_name);
    ElfRunner(const ElfRunner& other) = delete;
    ElfRunner& operator=(const ElfRunner& other) = delete;
    ElfRunner(ElfRunner&& other) = default;
    ElfRunner& operator=(ElfRunner&& other) = default;
    ~ElfRunner();

public:
    void run_functions(const std::vector<NamedSymbol>& functions);
    void run_function(const NamedSymbol& function, const std::vector<Address>& calls);
    void reset();

    bool is_dead() const;
    RuntimeArguments get_runtime_arguments() const;
    RuntimeRegs get_runtime_regs() const;
    RuntimeStacks get_runtime_stacks() const;

private:
    pid_t run(std::string file_name);
    void _update_is_dead(int child_status);
    void _log_step();
    void _log_function_arguments(const std::vector<NamedSymbol>& functions, Address function_address);
    
private:
    std::string _file_name;
    pid_t _child_pid;
    std::vector<BreakpointHook> _breakpoints;
    RuntimeRegs _runtime_regs;
    RuntimeStacks _runtime_stacks;
    RuntimeArguments _runtime_arguments;
    bool _is_dead;
};

