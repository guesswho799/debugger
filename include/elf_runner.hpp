#pragma once

#include <string>
#include <map>

class ElfRunner
{
public:
    explicit ElfRunner(std::string file_name);
    ~ElfRunner();

    std::map<int, int> run_function(uint64_t address, uint64_t size);

private:
    pid_t run(std::string file_name);
    void get_to_address(uint64_t address);
    
private:
    pid_t child_pid;
};
