#include <iostream>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include "status.hpp"

void log_function(std::vector<Disassebler::Line> lines, std::map<int, int> runtime_data)
{
    std::cout << "[counter] address: opcodes | assembly" << std::endl;
    for (const auto& line : lines)
    {
        std::cout << "[" << runtime_data[line.address] << "] ";
        std::cout << line.address << ": ";
        for (const auto& opcode: line.opcodes)
        {
            std::cout << std::hex << opcode << " ";
        }
        std::cout << line.disassemble << "\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "usage: ./Debugger [program name] [function name]" << std::endl;
        return 1;
    }

    try
    {
        const std::string program_name{argv[1]};
        const std::string function_name{argv[2]};
        ElfReader static_debugger{program_name};
        ElfRunner dynamic_debugger{program_name};

        const auto main = static_debugger.get_function(function_name);
        std::cout << "function found!\n" << main << std::endl;
        const auto section = static_debugger.get_section(main.section_index);
        std::cout << section << std::endl;

        auto address_counter = dynamic_debugger.run_function(main.value, main.size);
        auto assembly = static_debugger.get_function_code(main);
        log_function(assembly, address_counter);
    }
    catch (const std::exception& exception)
    {
        std::cout << "cought exception: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}
