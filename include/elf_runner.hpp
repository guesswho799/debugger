#pragma once

#include "breakpoint_hook.hpp"
#include <string>
#include <map>
#include <cstdint>
#include <optional>
#include <vector>


class ElfRunner
{
public:
    using address_t = uint64_t;
    using address_counter_t = uint64_t;
    using runtime_mapping = std::map<address_t, address_counter_t>;

public:
    explicit ElfRunner(std::string file_name);
    ElfRunner(const ElfRunner& other) = delete;
    ElfRunner& operator=(const ElfRunner& other) = delete;
    ElfRunner(ElfRunner&& other) = default;
    ElfRunner& operator=(ElfRunner&& other) = default;
    ~ElfRunner();

public:
    std::optional<runtime_mapping> run_function(uint64_t address, uint64_t size, std::vector<uint64_t> calls);
    void reset();
    bool is_dead();

private:
    pid_t run(std::string file_name);
    void _update_is_dead(int child_status);
    bool _non_blocking_get_to_address(uint64_t address, int child_status, std::vector<uint64_t> calls);
    void _step();
    
private:
    std::string _file_name;
    pid_t _child_pid;
    std::vector<BreakpointHook> _breakpoints;
    runtime_mapping _runtime_mapping;
    bool _is_dead;
};

