#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <optional>

#include "elf_header.hpp"


class ElfReader
{
public:
    explicit ElfReader(std::string file_name);
    ~ElfReader() = default;

    std::optional<NamedSection> get_section(std::string section_name);
    std::optional<NamedSection> get_section(std::size_t section_index);
    std::vector<NamedSymbol> get_symbols();
    std::vector<NamedSymbol> get_functions();
    NamedSymbol get_function(std::string name);
    std::vector<Disassebler::Line> get_function_code(NamedSymbol function);

private:
    ElfHeader header_factory();
    std::vector<NamedSection> sections_factory();
    std::vector<NamedSymbol> static_symbols_factory();

private:
    std::ifstream file;
public:
    ElfHeader header;
    std::vector<NamedSection> sections;
    std::vector<NamedSymbol> static_symbols;
};
