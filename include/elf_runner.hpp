#pragma once

#include "breakpoint_hook.hpp"
#include <string>
#include <map>
#include <cstdint>
#include <optional>


class ElfRunner
{
public:
    using runtime_mapping = std::map<uint64_t, int>;

public:
    explicit ElfRunner(std::string file_name);
    ElfRunner(const ElfRunner& other) = delete;
    ElfRunner& operator=(const ElfRunner& other) = delete;
    ElfRunner(ElfRunner&& other) = default;
    ElfRunner& operator=(ElfRunner&& other) = default;
    ~ElfRunner();

public:
    std::optional<runtime_mapping> run_function(uint64_t address, uint64_t size);

private:
    pid_t run(std::string file_name);
    bool get_to_address(uint64_t address);
    
private:
    pid_t child_pid;
    std::optional<BreakpointHook> _breakpoint;
};

