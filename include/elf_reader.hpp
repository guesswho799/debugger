#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <sys/types.h>
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
  bool is_position_independent() const;
  bool does_section_exist(const std::string_view &section_name) const;
  NamedSection get_section(const std::string_view &section_name) const;
  NamedSection get_section(std::size_t section_index) const;
  size_t get_section_index(const std::string_view &section_name) const;
  NamedSymbol get_function(std::string name, uint64_t base_address) const;
  std::vector<NamedSymbol> get_functions() const;
  std::vector<NamedSymbol> get_implemented_functions(uint64_t base_address) const;
  std::vector<uint64_t> get_function_calls(std::string name, uint64_t base_address) const;
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
  template <typename It>
  std::pair<int, size_t> find_next_start_of_function(It begin, It end);
  std::vector<NamedSymbol> fake_static_symbols_factory();
  std::vector<NamedSymbol> static_symbols_factory();
  std::vector<NamedSymbol> dynamic_symbols_factory();
  std::vector<std::string> strings_factory();
  bool _is_valid_string(const std::string &s);

private:
  mutable std::ifstream _file;
  std::string _file_name;
  ElfHeader _header;
  std::vector<NamedSection> _sections;
  std::vector<NamedSymbol> _static_symbols;
  std::vector<NamedSymbol> _dynamic_symbols;
  std::vector<std::string> _strings;

private:
  static constexpr std::string_view start_of_function_instruction = "endbr64";
  static constexpr std::string_view code_section_name = ".text";
  static constexpr std::string_view static_symbol_section_name = ".symtab";
  static constexpr std::string_view static_symbol_name_section_name = ".strtab";
  static constexpr std::string_view dynamic_symbol_section_name = ".dynsym";
  static constexpr std::string_view dynamic_symbol_name_section_name =
      ".dynstr";
};
