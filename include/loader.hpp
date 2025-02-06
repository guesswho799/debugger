#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include <ftxui/dom/table.hpp>

namespace Loader {
static constexpr size_t opcodes_per_line = 3;
std::string convert_to_hex(uint64_t number);
ftxui::Element get_opcodes(const Disassembler::Line &line);
std::pair<int, uint64_t>
max_instruction_height(const std::vector<Disassembler::Line> &assembly,
                       uint64_t scroll_bar, bool is_single_trace);
size_t max_instruction_width(const std::vector<Disassembler::Line> &assembly);
ftxui::Element
load_instructions(const std::string &function_name,
                  const std::vector<Disassembler::Line> &assembly,
                  uint64_t scroll_bar,
                  std::optional<uint64_t> line_selected = {});
ftxui::Element load_register_window(const struct user_regs_struct &registers);
ftxui::Element load_stack_window(const ElfRunner::RuntimeStack &stack_metadata);
ftxui::Element load_trace_player(const ElfRunner::RuntimeRegs &runtime_data,
                                 uint64_t code_selector);
std::vector<std::vector<std::string>>
load_function_table(const ElfReader &static_debugger);
ftxui::Element load_strings(const std::vector<ElfString> &strings);
ftxui::Element
load_functions_arguments(const ElfRunner::RuntimeArguments runtime_value);
} // namespace Loader
