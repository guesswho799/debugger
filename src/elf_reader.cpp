#include "elf_reader.hpp"
#include "status.hpp"
#include <algorithm>
#include <cstdint>
#include <ranges>

// constructors
ElfReader::ElfReader(std::string file_name)
    : _file(std::ifstream(file_name)), _file_name(file_name),
      _header(header_factory()), _sections(sections_factory()),
      _static_symbols(static_symbols_factory()),
      _dynamic_symbols(dynamic_symbols_factory()), _strings(strings_factory()) {
}

ElfReader::ElfReader(ElfReader &&other)
    : _file(std::ifstream(other._file_name)), _file_name(other._file_name),
      _header(other.get_header()), _sections(other.get_sections()),
      _static_symbols(other.get_static_symbols()),
      _dynamic_symbols(other.get_dynamic_symbols()),
      _strings(other.get_strings()) {}

ElfReader &ElfReader::operator=(ElfReader &&other) {
  _file = std::ifstream(other._file_name);
  _file_name = other._file_name;
  _header = other.get_header();
  _sections = other.get_sections();
  _static_symbols = other.get_static_symbols();
  _dynamic_symbols = other.get_dynamic_symbols();
  _strings = other.get_strings();
  return *this;
}

ElfReader::~ElfReader() { _file.close(); }

// geters
ElfHeader ElfReader::get_header() const { return _header; }

std::vector<NamedSection> ElfReader::get_sections() const { return _sections; }

std::vector<NamedSymbol> ElfReader::get_static_symbols() const {
  return _static_symbols;
}

std::vector<NamedSymbol> ElfReader::get_dynamic_symbols() const {
  return _dynamic_symbols;
}

std::vector<std::string> ElfReader::get_strings() const { return _strings; }

// filtered geters
NamedSection
ElfReader::get_section(const std::string_view &section_name) const {
  for (const auto &section : _sections) {
    if (section.name == section_name) {
      return section;
    }
  }

  throw CriticalException(Status::elf_header__section_not_found);
}

NamedSection ElfReader::get_section(std::size_t section_index) const {
  if (_sections.size() < section_index) {
    throw CriticalException(Status::elf_header__section_not_found);
  }

  return _sections[section_index];
}

NamedSymbol ElfReader::get_function(std::string name) const {
  auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function && symbol.name == name;
  };
  const auto iterator = std::find_if(_static_symbols.begin(),
                                     _static_symbols.end(), function_filter);
  if (iterator == _static_symbols.end())
    throw CriticalException(Status::elf_header__function_not_found);

  return *iterator;
}

std::vector<NamedSymbol> ElfReader::get_functions() const {
  std::vector<NamedSymbol> functions{};
  auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function;
  };
  for (const auto &symbol :
       _static_symbols | std::views::filter(function_filter)) {
    functions.push_back(symbol);
  }
  return functions;
}

std::vector<NamedSymbol> ElfReader::get_implemented_functions() const {
  std::vector<NamedSymbol> functions{};
  auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function and symbol.value != 0;
  };
  for (const auto &symbol :
       _static_symbols | std::views::filter(function_filter)) {
    functions.push_back(symbol);
  }
  return functions;
}

std::vector<uint64_t> ElfReader::get_function_calls(std::string name) const {
  std::vector<Disassembler::Line> lines = get_function_code_by_name(name);
  std::vector<uint64_t> calls;
  constexpr uint64_t CALL_OPCODE = 0xE8;
  const uint64_t amount_of_lines = lines.end() - lines.begin();
  for (uint64_t i = 0; i < amount_of_lines; i++) {
    if (lines[i].opcodes[0] == CALL_OPCODE and i < amount_of_lines - 1)
      calls.push_back(lines[i + 1].address);
  }
  return calls;
}

std::vector<Disassembler::Line>
ElfReader::get_function_code(NamedSymbol function) const {
  if (!_file.is_open()) {
    throw CriticalException(Status::elf_header__open_failed);
  }

  uint64_t offset = function.value -
                    _sections[function.section_index].loaded_virtual_address +
                    _sections[function.section_index].unloaded_offset;
  _file.seekg(static_cast<long>(offset));

  std::vector<unsigned char> buffer(function.size);
  _file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

  Disassembler disassembler{};
  return disassembler.disassemble(buffer.data(), buffer.size(), function.value,
                                  get_static_symbols());
}

std::vector<Disassembler::Line>
ElfReader::get_function_code_by_name(std::string name) const {
  return get_function_code(get_function(name));
}

// factories
ElfHeader ElfReader::header_factory() {
  if (!_file.is_open()) {
    throw CriticalException(Status::elf_header__open_failed);
  }

  ElfHeader elf_header{};
  _file.read(reinterpret_cast<char *>(&elf_header), sizeof elf_header);

  return elf_header;
}

std::vector<NamedSection> ElfReader::sections_factory() {
  if (!_file.is_open()) {
    throw CriticalException(Status::elf_header__open_failed);
  }

  _file.seekg(static_cast<long>(_header.section_table_address));

  std::vector<SectionHeader> sections{};
  for (int i = 0; i < _header.section_table_entry_count; i++) {
    SectionHeader section{};
    _file.read(reinterpret_cast<char *>(&section), sizeof section);

    sections.push_back(section);
  }

  std::vector<NamedSection> named_sections{};
  for (const auto &section : sections) {
    _file.seekg(sections[_header.section_table_name_index].unloaded_offset +
                section.name_offset);
    std::string name;
    std::getline(_file, name, '\0');
    named_sections.emplace_back(
        name, section.type, section.attributes, section.loaded_virtual_address,
        section.unloaded_offset, section.size, section.associated_section_index,
        section.extra_information, section.required_alinment,
        section.entry_size);
  }

  return named_sections;
}

std::vector<NamedSymbol>
ElfReader::symbols_factory(const std::string_view &section_name,
                           const std::string_view &string_table_name) {
  if (!_file.is_open()) {
    throw CriticalException(Status::elf_header__open_failed);
  }

  const NamedSection symbol_table = get_section(section_name);

  _file.seekg(symbol_table.unloaded_offset);

  std::vector<ElfSymbol> symbols{};
  ElfSymbol symbol{};
  while (static_cast<uint64_t>(_file.tellg()) <
         symbol_table.unloaded_offset + symbol_table.size) {
    _file.read(reinterpret_cast<char *>(&symbol), sizeof symbol);
    symbols.push_back(symbol);
  }

  const NamedSection str_table = get_section(string_table_name);
  std::vector<NamedSymbol> named_symbols{};
  for (const auto &symbol : symbols) {
    _file.seekg(str_table.unloaded_offset + symbol.name);
    std::string name;
    std::getline(_file, name, '\0');
    named_symbols.emplace_back(name, static_cast<SymbolType>(symbol.type),
                               symbol.section_index, symbol.value, symbol.size);
  }

  return named_symbols;
}

std::vector<NamedSymbol> ElfReader::static_symbols_factory() {
  return symbols_factory(".symtab", ".strtab");
}

std::vector<NamedSymbol> ElfReader::dynamic_symbols_factory() {
  return symbols_factory(".dynsym", ".dynstr");
}

std::vector<std::string> ElfReader::strings_factory() {
  if (!_file.is_open()) {
    throw CriticalException(Status::elf_header__open_failed);
  }

  const NamedSection string_section = get_section(".rodata");
  _file.seekg(static_cast<long>(string_section.unloaded_offset));
  std::vector<std::string> strings;
  while (static_cast<uint64_t>(_file.tellg()) <
         string_section.unloaded_offset + string_section.size) {
    std::string string;
    bool skip_irrelevant_strings = false;
    std::getline(_file, string, '\0');

    if (string.length() == 0)
      skip_irrelevant_strings = true;
    for (const char &character : string)
      if (!isascii(character))
        skip_irrelevant_strings = true;
    if (skip_irrelevant_strings)
      continue;

    strings.push_back(string);
  }

  return strings;
}
