#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "disassembler.hpp"
#include "elf_header.hpp"

class ElfReader {
  // constructors
public:
  explicit ElfReader(std::string file_name);
  ElfReader(const ElfReader &other) = delete;
  ElfReader &operator=(const ElfReader &other) = delete;
  ElfReader(ElfReader &&other);
  ElfReader &operator=(ElfReader &&other);
  ~ElfReader();

  // geters
public:
  ElfHeader get_header() const;
  std::vector<NamedSection> get_sections() const;
  std::vector<NamedSymbol> get_static_symbols() const;
  std::vector<NamedSymbol> get_dynamic_symbols() const;
  std::vector<std::string> get_strings() const;

  // filtered geters
public:
  NamedSection get_section(const std::string_view &section_name) const;
  NamedSection get_section(std::size_t section_index) const;
  NamedSymbol get_function(std::string name) const;
  std::vector<NamedSymbol> get_functions() const;
  std::vector<NamedSymbol> get_implemented_functions() const;
  std::vector<uint64_t> get_function_calls(std::string name) const;
  std::vector<Disassembler::Line> get_function_code(NamedSymbol function) const;
  std::vector<Disassembler::Line>
  get_function_code_by_name(std::string name) const;

  // factories
private:
  ElfHeader header_factory();
  std::vector<NamedSection> sections_factory();
  std::vector<NamedSymbol>
  symbols_factory(const std::string_view &section_name,
                  const std::string_view &string_table_name);
  std::vector<NamedSymbol> static_symbols_factory();
  std::vector<NamedSymbol> dynamic_symbols_factory();
  std::vector<std::string> strings_factory();

private:
  mutable std::ifstream _file;
  std::string _file_name;
  ElfHeader _header;
  std::vector<NamedSection> _sections;
  std::vector<NamedSymbol> _static_symbols;
  std::vector<NamedSymbol> _dynamic_symbols;
  std::vector<std::string> _strings;
};
