#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ranges>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "status.hpp"
#include "elf_header.hpp"


ElfReader::ElfReader(std::string file_name):
    file(std::ifstream(file_name)),
    header(header_factory()),
    sections(sections_factory()),
    _static_symbols(static_symbols_factory())
{}

ElfHeader ElfReader::header_factory()
{
    if (!file.is_open())
    {
        throw CriticalException(Status::elf_header__open_failed);
    }

    ElfHeader elf_header{};
    file.read(reinterpret_cast<char*>(&elf_header), sizeof elf_header);

    return elf_header;
}

std::vector<NamedSection> ElfReader::sections_factory()
{
    if (!file.is_open())
    {
        throw CriticalException(Status::elf_header__open_failed);
    }

    file.seekg(header.section_table_adress);

    std::vector<SectionHeader> sections{}; 
    for (int i = 0; i < header.section_table_entry_count; i++)
    {
        SectionHeader section{};
        file.read(reinterpret_cast<char*>(&section), sizeof section);
       
        sections.push_back(section);
    }

    std::vector<NamedSection> named_sections{};
    NamedSection tmp_section{};
    for (const auto& section : sections)
    {
        file.seekg(sections[header.section_table_name_index].unloaded_offset+section.name_offset);
        std::getline(file, tmp_section.name, '\0');
        tmp_section.type = section.type;
        tmp_section.attributes = section.attributes;
        tmp_section.loaded_virtual_address = section.loaded_virtual_address;
        tmp_section.unloaded_offset = section.unloaded_offset;
        tmp_section.size = section.size;
        tmp_section.associated_section_index = section.associated_section_index;
        tmp_section.extra_information = section.extra_information;
        tmp_section.required_alinment = section.required_alinment;
        tmp_section.entry_size = section.entry_size;
        named_sections.push_back(tmp_section);
    }

    return named_sections;
}

NamedSection ElfReader::get_section(std::string section_name)
{
    for (const auto& section : sections)
    {
        if (section.name == section_name)
        {
            return section;
        }
    }

    throw CriticalException(Status::elf_header__section_not_found);
}

NamedSection ElfReader::get_section(std::size_t section_index)
{
    if (sections.size() < section_index)
    {
        throw CriticalException(Status::elf_header__section_not_found);
    }

    return sections[section_index];
}

std::vector<NamedSymbol> ElfReader::get_symbols()
{
    return _static_symbols;
}

std::vector<NamedSymbol> ElfReader::static_symbols_factory()
{
    if (!file.is_open())
    {
        throw CriticalException(Status::elf_header__open_failed);
    }

    const NamedSection symbol_table = get_section(".symtab");

    file.seekg(symbol_table.unloaded_offset);

    std::vector<ElfSymbol> symbols{};
    ElfSymbol symbol{};
    while (static_cast<uint64_t>(file.tellg()) < symbol_table.unloaded_offset + symbol_table.size)
    {
        file.read(reinterpret_cast<char*>(&symbol), sizeof symbol);
        symbols.push_back(symbol);
    }

    NamedSection str_table = get_section(".strtab");
    NamedSymbol named_symbol{};
    std::vector<NamedSymbol> named_symbols{};
    for (const auto& symbol : symbols)
    {
        file.seekg(str_table.unloaded_offset + symbol.name);
        std::getline(file, named_symbol.name, '\0');

        named_symbol.type = static_cast<SymbolType>(symbol.type);
        named_symbol.section_index = symbol.section_index;
        named_symbol.value = symbol.value;
        named_symbol.size = symbol.size;
        named_symbols.push_back(named_symbol);
    }

    return named_symbols;
}

std::vector<NamedSymbol> ElfReader::get_functions()
{
    std::vector<NamedSymbol> functions{};
    auto function_filter = [&](NamedSymbol symbol){return symbol.type & SymbolType::function; };
    for(const auto& symbol : _static_symbols | std::views::filter(function_filter))
    {
        functions.push_back(symbol);
    }
    return functions;
}

NamedSymbol ElfReader::get_function(std::string name)
{
    auto function_filter = [&](NamedSymbol symbol){return symbol.type & SymbolType::function && symbol.name == name; };
    for(const auto& symbol : _static_symbols | std::views::filter(function_filter))
    {
        return symbol;
    }
    throw CriticalException(Status::elf_header__function_not_found);
}

std::vector<Disassebler::Line> ElfReader::get_function_code(NamedSymbol function)
{
    if (!file.is_open())
    {
        throw CriticalException(Status::elf_header__open_failed);
    }

    int offset = function.value - sections[function.section_index].loaded_virtual_address + sections[function.section_index].unloaded_offset;
    file.seekg(offset);

    unsigned char buffer[function.size]{};
    file.read(reinterpret_cast<char*>(buffer), sizeof buffer);
    std::vector<uint8_t> code{buffer, buffer+function.size};

    return Disassebler::disassemble_raw(code.data(), code.size(), function.value);
}
