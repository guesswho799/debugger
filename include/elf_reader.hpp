#pragma once

#include <string>
#include <fstream>
#include <vector>

#include "disassembler.hpp"
#include "elf_header.hpp"


class ElfReader
{
public:
    explicit ElfReader(std::string file_name);
    ElfReader(const ElfReader& other) = delete;
    ElfReader& operator=(const ElfReader& other) = delete;

    ElfReader(ElfReader&& other):
    file(std::ifstream(other._file_name)),
    _file_name(other._file_name),
    header(header_factory()),
    sections(sections_factory()),
    _static_symbols(static_symbols_factory()),
    _strings(strings_factory())
    {
         other.file.close();
    }
    ElfReader& operator=(ElfReader&& other)
    {
         file = std::ifstream(other._file_name);
         _file_name = other._file_name;
         header = header_factory();
         sections = sections_factory();
         _static_symbols = static_symbols_factory();
         _strings = strings_factory();
         other.file.close();
         return *this;
    }
    ~ElfReader() = default;

    NamedSection get_section(std::string section_name) const;
    NamedSection get_section(std::size_t section_index) const;
    std::vector<NamedSymbol> get_symbols() const;
    std::vector<std::string> get_strings() const;
    std::vector<NamedSymbol> get_functions() const;
    NamedSymbol get_function(std::string name) const;
    std::vector<Disassembler::Line> get_function_code(NamedSymbol function) const;
    std::vector<Disassembler::Line> get_function_code_by_name(std::string name) const;
    std::vector<uint64_t> get_function_calls(std::string name) const;

private:
    ElfHeader header_factory();
    std::vector<NamedSection> sections_factory();
    std::vector<NamedSymbol> static_symbols_factory();
    std::vector<std::string> strings_factory();

private:
    mutable std::ifstream file;

public:
    std::string _file_name;
    ElfHeader header;
    std::vector<NamedSection> sections;
    std::vector<NamedSymbol> _static_symbols;
    std::vector<std::string> _strings;
};
