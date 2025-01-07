#pragma once

#include "breakpoint_hook.hpp"
#include "elf_header.hpp"

#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <vector>


class ElfRunner
{
public:
    using address_t         = uint64_t;
    using address_counter_t = uint64_t;
    using runtime_mapping   = std::map<address_t, address_counter_t>;
    using runtime_arguments = std::map<std::string, int64_t>;

public:
    explicit ElfRunner(std::string file_name);
    ElfRunner(const ElfRunner& other) = delete;
    ElfRunner& operator=(const ElfRunner& other) = delete;
    ElfRunner(ElfRunner&& other) = default;
    ElfRunner& operator=(ElfRunner&& other) = default;
    ~ElfRunner();

public:
    void run_functions(const std::vector<NamedSymbol>& functions);
    std::optional<runtime_arguments> get_runtime_arguments() const;
    void reset();
    bool is_dead();

private:
    pid_t run(std::string file_name);
    void _update_is_dead(int child_status);
    void _log_step();
    void _log_function_arguments(const std::vector<NamedSymbol>& functions, address_t function_address);
    
private:
    std::string _file_name;
    pid_t _child_pid;
    std::vector<BreakpointHook> _breakpoints;
    runtime_mapping _runtime_mapping;
    runtime_arguments _runtime_arguments;
    bool _is_dead;
};

