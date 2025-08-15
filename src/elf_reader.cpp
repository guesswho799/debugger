#include "elf_reader.hpp"
#include "disassembler.hpp"
#include "elf_header.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <elf.h>
#include <format>
#include <fstream>
#include <iterator>
#include <locale>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

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

std::vector<ElfString> ElfReader::get_strings() const { return _strings; }

// filtered geters
bool ElfReader::is_position_independent() const {
  return _header.file_type == ET_DYN;
}

bool ElfReader::does_section_exist(const std::string_view &section_name) const {
  for (const auto &section : _sections) {
    if (section.name == section_name) {
      return true;
    }
  }
  return false;
}

NamedSection
ElfReader::get_section(const std::string_view &section_name) const {
  for (const auto &section : _sections) {
    if (section.name == section_name) {
      return section;
    }
  }

  throw std::runtime_error("elf header section not found");
}

size_t
ElfReader::get_section_index(const std::string_view &section_name) const {
  size_t index = 0;
  for (const auto &section : _sections) {
    if (section.name == section_name) {
      return index;
    }
    index++;
  }

  throw std::runtime_error("elf header section not found");
}

NamedSection ElfReader::get_section(std::size_t section_index) const {
  if (_sections.size() < section_index) {
    throw std::runtime_error("elf header section not found");
  }

  return _sections[section_index];
}

NamedSymbol ElfReader::get_function(std::string name,
                                    uint64_t base_address) const {
  const auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function && symbol.name == name;
  };
  auto iterator = std::find_if(_static_symbols.begin(), _static_symbols.end(),
                               function_filter);
  if (iterator == _static_symbols.end())
    throw std::runtime_error("elf header function not found");

  NamedSymbol result = *iterator;
  result.value += base_address;

  return result;
}

std::vector<NamedSymbol> ElfReader::get_functions() const {
  std::vector<NamedSymbol> functions{};
  const auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function;
  };
  for (const auto &symbol :
       _static_symbols | std::views::filter(function_filter)) {
    functions.push_back(symbol);
  }
  return functions;
}

std::vector<NamedSymbol>
ElfReader::get_implemented_functions(uint64_t base_address) const {
  std::vector<NamedSymbol> functions{};
  const auto function_filter = [&](const NamedSymbol &symbol) {
    return symbol.type & SymbolType::function and symbol.value != 0;
  };
  for (const auto &symbol :
       _static_symbols | std::views::filter(function_filter)) {
    functions.push_back(symbol);
  }

  if (base_address != 0)
    for (auto &function : functions)
      function.value += base_address;

  return functions;
}

std::vector<uint64_t>
ElfReader::get_function_calls(std::string name, uint64_t base_address) const {
  std::vector<Disassembler::Line> lines = get_function_code_by_name(name);
  std::vector<uint64_t> calls;
  constexpr uint64_t CALL_OPCODE = 0xE8;
  const uint64_t amount_of_lines = lines.end() - lines.begin();

  for (uint64_t i = 0; i < amount_of_lines; i++) {
    if (lines[i].opcodes[0] == CALL_OPCODE and i < amount_of_lines - 1)
      calls.push_back(lines[i + 1].address + base_address);
  }
  return calls;
}

std::vector<Disassembler::Line>
ElfReader::get_function_code(const NamedSymbol &function,
                             bool try_resolve) const {
  if (!_file.is_open()) {
    throw std::runtime_error("elf header open failed");
  }

  const uint64_t offset =
      function.value + _sections[function.section_index].unloaded_offset -
      _sections[function.section_index].loaded_virtual_address;
  _file.seekg(static_cast<long>(offset));

  std::vector<unsigned char> buffer(function.size);
  _file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

  Disassembler disassembler{};
  if (try_resolve)
    return disassembler.disassemble(buffer, function.value,
                                    get_static_symbols(), get_dynamic_symbols(),
                                    get_strings());
  else
    return disassembler.disassemble(buffer, function.value);
}

std::vector<Disassembler::Line>
ElfReader::get_function_code_by_name(std::string name) const {
  return get_function_code(get_function(name, 0), true);
}

// factories
ElfHeader ElfReader::header_factory() {
  if (!_file.is_open()) {
    throw std::runtime_error("elf header open failed");
  }

  ElfHeader elf_header{};
  _file.read(reinterpret_cast<char *>(&elf_header), sizeof elf_header);

  return elf_header;
}

std::vector<NamedSection> ElfReader::sections_factory() {
  if (!_file.is_open()) {
    throw std::runtime_error("elf header open failed");
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
    throw std::runtime_error("elf header open failed");
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

std::vector<NamedSymbol> ElfReader::fake_static_symbols_factory() {
  if (!_file.is_open()) {
    throw std::runtime_error("elf header open failed");
  }

  const NamedSection code_section = get_section(code_section_name);
  _file.seekg(static_cast<long>(code_section.unloaded_offset));

  std::vector<unsigned char> buffer(code_section.size);
  _file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

  Disassembler disassembler{};
  const std::vector<Disassembler::Line> lines =
      disassembler.disassemble(buffer, code_section.unloaded_offset);

  const size_t section_index = get_section_index(code_section_name);
  const uint64_t section_address = code_section.loaded_virtual_address;
  const uint64_t entry_point = _header.entry_point_address;
  constexpr SymbolType symbol_type = SymbolType::function;
  std::vector<NamedSymbol> symbols{};
  size_t section_offset = 0;
  auto line = lines.begin();

  while (line != lines.end()) {
    const auto [amount_of_instructions, function_size] =
        find_next_start_of_function(line, lines.end());
    const uint64_t address = section_address + section_offset;
    const std::string function_name =
        address == entry_point ? "_start"
                               : std::format("function_{:x}", address);

    symbols.emplace_back(function_name, symbol_type, section_index, address,
                         function_size);

    section_offset += function_size;
    line += amount_of_instructions;
  }

  return symbols;
}

template <typename It>
std::pair<int, size_t> ElfReader::find_next_start_of_function(It begin,
                                                              It end) {
  int amount_of_instructions = 0;
  size_t function_size = 0;
  int amount_of_function_begin_passed = 0;
  for (; begin != end; ++begin) {
    if (begin->instruction == start_of_function_instruction)
      if (++amount_of_function_begin_passed == 2)
        break;

    amount_of_instructions++;
    function_size += begin->opcodes.end() - begin->opcodes.begin();
  }
  return std::make_pair(amount_of_instructions, function_size);
}

std::vector<NamedSymbol> ElfReader::static_symbols_factory() {
  if (does_section_exist(static_symbol_section_name))
    return symbols_factory(static_symbol_section_name,
                           static_symbol_name_section_name);
  return fake_static_symbols_factory();
}

std::vector<NamedSymbol> ElfReader::dynamic_symbols_factory() {
  auto dynamic_symbols = symbols_factory(dynamic_symbol_section_name,
                                         dynamic_symbol_name_section_name);
  resolve_dynamic_symbols_address(dynamic_symbols);
  return dynamic_symbols;
}

void ElfReader::resolve_dynamic_symbols_address(
    std::vector<NamedSymbol> &symbols) {
  constexpr int jmp_offset = 1;
  constexpr auto plt_entry_size = 16;
  const auto plt_section = get_section(relocation_plt_section_name);
  const auto plt_index =
      static_cast<uint16_t>(get_section_index(relocation_plt_section_name));
  uint64_t counter = 0;
  const std::string name;

  struct plt_entry {
    uint64_t entry_address;
    uint64_t entry_jump_to;
  };
  std::vector<plt_entry> plt_entries;

  while (static_cast<uint64_t>(_file.tellg()) <
         plt_section.unloaded_offset + plt_section.size) {
    const uint64_t entry_address =
        plt_section.unloaded_offset + counter * plt_entry_size;
    const NamedSymbol entry{.name = name,
                            .type = SymbolType::function,
                            .section_index = plt_index,
                            .value = entry_address,
                            .size = plt_entry_size};
    const Disassembler::Line entry_code =
        get_function_code(entry, false)[jmp_offset];

    const int64_t relative_jump_address =
        Disassembler::get_address(entry_code.arguments);
    const uint64_t entry_code_size =
        entry_code.opcodes.end() - entry_code.opcodes.begin();
    const uint64_t absolute_jump_address =
        entry_code.address + entry_code_size + relative_jump_address;
    plt_entries.emplace_back(entry_address, absolute_jump_address);
    counter++;
  }

  ElfRelocation relocation_info{};
  const auto relocation_info_section =
      get_section(relocation_plt_symbol_info_section_name);
  _file.seekg(static_cast<long>(relocation_info_section.unloaded_offset));

  while (static_cast<uint64_t>(_file.tellg()) <
         relocation_info_section.unloaded_offset +
             relocation_info_section.size) {
    _file.read(reinterpret_cast<char *>(&relocation_info),
               sizeof relocation_info);
    for (const auto &entry : plt_entries) {
      if (entry.entry_jump_to == relocation_info.file_offset) {
        symbols[relocation_info.symbol_index].value = entry.entry_address;
        break;
      }
    }
  }
}

std::vector<ElfString> ElfReader::strings_factory() {
  if (!_file.is_open()) {
    throw std::runtime_error("elf header open failed");
  }

  const NamedSection string_section = get_section(".rodata");
  _file.seekg(static_cast<long>(string_section.unloaded_offset));
  std::vector<ElfString> strings;
  while (static_cast<uint64_t>(_file.tellg()) <
         string_section.unloaded_offset + string_section.size) {
    const auto address = static_cast<uint64_t>(_file.tellg());
    const auto value = get_next_string(string_section);
    if (_is_valid_string(value)) {
      std::string v(value.begin(), value.end());
      strings.emplace_back(v, address);
    }
  }

  return strings;
}

std::vector<char>
ElfReader::get_next_string(const NamedSection &string_section) {
  char byte_read;
  std::vector<char> result;
  while (static_cast<uint64_t>(_file.tellg()) <
         string_section.unloaded_offset + string_section.size) {
    _file.read(reinterpret_cast<char *>(&byte_read), sizeof byte_read);

    if (byte_read == 0)
      break;

    result.push_back(byte_read);
  }
  return result;
}

bool ElfReader::_is_valid_string(const std::vector<char> &s) {
  if (s.size() == 0)
    return false;

  bool is_all_whitespace = true;
  for (const auto &character : s) {
    if (!std::isprint(character) and character != '\n')
      return false;

    if (is_all_whitespace) {
      is_all_whitespace = isspace(character);
    }
  }

  if (is_all_whitespace)
    return false;

  return true;
}
