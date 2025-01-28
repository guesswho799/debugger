#include "loader.hpp"

#include <cmath>
#include <ranges>
#include <sstream>
#include <vector>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include <ftxui/dom/table.hpp>

namespace Loader {
std::string convert_to_hex(uint64_t number) {
  std::ostringstream stream;
  stream << "0x" << std::hex << number;
  return stream.str();
}

ftxui::Element get_opcodes(const Disassembler::Line &line) {
  auto opcodes_as_box = ftxui::vbox({});
  std::ostringstream opcodes_stream;
  const int opcodes_per_line = 3;
  for (unsigned int i = 0; i < line.opcodes.size(); i++) {
    if (i % opcodes_per_line == 0 && i != 0) {
      opcodes_as_box =
          ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
      opcodes_stream.str("");
      opcodes_stream.clear();
    }
    if (line.opcodes[i] < 16)
      opcodes_stream << '0';
    opcodes_stream << std::hex << line.opcodes[i];
    if ((i + 1) % opcodes_per_line != 0 or i == 0)
      opcodes_stream << ' ';
  }
  if ((line.opcodes.end() - line.opcodes.begin()) % opcodes_per_line != 0) {
    for (unsigned int i = 1;
         i < (opcodes_per_line - ((line.opcodes.end() - line.opcodes.begin()) %
                                  opcodes_per_line)) *
                 opcodes_per_line;
         i++) {
      opcodes_stream << ' ';
    }
  }
  return ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
}

std::pair<int, int>
max_instruction_height(const std::vector<Disassembler::Line> &assembly,
                       uint64_t scroll_bar, bool is_single_trace) {
  int result = 0;
  const size_t screen_height = ftxui::Terminal::Size().dimy;
  const size_t main_display_height =
      is_single_trace ? screen_height - 12 : screen_height - 6;
  int height_counter = 0;
  constexpr float opcodes_per_line = 3;

  for (const auto &line : assembly | std::views::drop(scroll_bar)) {
    const float amount_of_opcodes = line.opcodes.end() - line.opcodes.begin();
    const float lines_of_opcodes =
        std::ceil(amount_of_opcodes / opcodes_per_line);
    if (height_counter + lines_of_opcodes >= main_display_height)
      break;
    height_counter += lines_of_opcodes;
    result++;
  }

  const int lines_left = assembly.size() > main_display_height
                             ? main_display_height - height_counter
                             : 0;
  return std::make_pair(result, lines_left);
}

int
max_instruction_width(const std::vector<Disassembler::Line> &assembly) {
  int max_line_width = 0;

  for (const auto &line : assembly) {
    const std::string address = convert_to_hex(line.address);
    const float amount_of_opcodes = line.opcodes.end() - line.opcodes.begin();
    const float opcodes_per_line = 3;
    const int width =
        address.length() + std::min(amount_of_opcodes, opcodes_per_line) * 2 +
        2 + line.instruction.length() + 1 + line.arguments.length() + 4;
    if (width > max_line_width)
      max_line_width = width;
  }
  return max_line_width;
}

ftxui::Element
load_instructions(const std::string &function_name,
                  const std::vector<Disassembler::Line> &assembly,
                  uint64_t scroll_bar,
                  std::optional<uint64_t> line_selected) {
  auto block =
      ftxui::vbox({ftxui::hbox({ftxui::text("Instructions"), ftxui::separator(),
                                ftxui::text(function_name) | ftxui::bold}),
                   ftxui::separator()});
  const auto [amount_of_instruction, lines_left] =
      max_instruction_height(assembly, scroll_bar, line_selected.has_value());

  for (const auto &line : assembly | std::views::drop(scroll_bar) |
                              std::views::take(amount_of_instruction)) {
    auto content =
        ftxui::hbox({ftxui::text(convert_to_hex(line.address)),
                     ftxui::separator(), get_opcodes(line), ftxui::separator(),
                     ftxui::text(line.instruction + " " + line.arguments)});
    if (line_selected.has_value() and line_selected.value() == line.address)
      content = content | ftxui::inverted;

    // TODO:: display jumps as arrows
    block = ftxui::vbox(block, content);
  }

  for (int i = 1; i < lines_left; i++)
    block = ftxui::vbox(block, ftxui::text(""));

  return block | ftxui::border |
         ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                     max_instruction_width(assembly)) |
         ftxui::center;
}

ftxui::Element
load_register_window(const struct user_regs_struct &registers) {
  std::vector<std::vector<std::string>> table_content{
      {"rax ", convert_to_hex(registers.rax)},
      {"rbx ", convert_to_hex(registers.rbx)},
      {"rcx ", convert_to_hex(registers.rcx)},
      {"rdx ", convert_to_hex(registers.rdx)},
      {"rdi ", convert_to_hex(registers.rdi)},
      {"rsi ", convert_to_hex(registers.rsi)},
      {"r8  ", convert_to_hex(registers.r8)},
      {"r9  ", convert_to_hex(registers.r9)},
      {"r10 ", convert_to_hex(registers.r10)},
      {"r11 ", convert_to_hex(registers.r11)},
      {"r12 ", convert_to_hex(registers.r12)},
      {"r13 ", convert_to_hex(registers.r13)},
      {"r14 ", convert_to_hex(registers.r14)},
      {"r15 ", convert_to_hex(registers.r15)},
      {"rip ", convert_to_hex(registers.rip)},
      {"rbp ", convert_to_hex(registers.rbp)},
      {"rsp ", convert_to_hex(registers.rsp)}};
  auto table = ftxui::Table(table_content);
  table.SelectColumn(0).Decorate(ftxui::color(ftxui::Color::White));
  table.SelectColumn(1).Decorate(ftxui::color(ftxui::Color::Grey50));

  return ftxui::vbox({ftxui::text("Registers") | ftxui::bold,
                      ftxui::separator(), table.Render()}) |
         ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 25) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 19) | ftxui::border;
}

ftxui::Element
load_stack_window(const ElfRunner::RuntimeStack &stack_metadata) {
  const auto &[base_stack_pointer, stack] = stack_metadata;
  auto block = ftxui::vbox({});
  for (int i = 0; i < ElfRunner::stack_size; i++) {
    std::string index_subtractor =
        i == 0 ? "   "
               : "- " + std::to_string(i * sizeof(ElfRunner::StackElement));
    if (index_subtractor.length() < 4)
      index_subtractor += " ";

    block = ftxui::vbox({ftxui::hbox({ftxui::text("rbp " + index_subtractor),
                                      ftxui::separator(),
                                      ftxui::text(convert_to_hex(stack[i]))}),
                         block});
  }
  return ftxui::vbox({ftxui::text("Stack"), ftxui::separator(), block}) |
         ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 12) | ftxui::border;
}

ftxui::Element
load_trace_player(const ElfRunner::RuntimeRegs &runtime_data,
                  uint64_t code_selector) {
  const float data_size = runtime_data.end() - runtime_data.begin() - 1;
  return ftxui::vbox(
             {ftxui::text("Player"), ftxui::separator(),
              ftxui::gauge(code_selector / data_size) |
                  ftxui::color(ftxui::Color::White),
              ftxui::text("<-- " + std::to_string(code_selector) + "/" +
                          std::to_string(static_cast<uint64_t>(data_size)) +
                          " -->") |
                  ftxui::center}) |
         ftxui::flex | ftxui::border;
}

std::vector<std::vector<std::string>>
load_function_table(const ElfReader &static_debugger) {
  std::vector<std::vector<std::string>> function_table_info;
  for (const auto &item : static_debugger.get_functions()) {
    function_table_info.push_back(
        {item.name, std::to_string(item.value), std::to_string(item.size)});
  }
  return function_table_info;
}

ftxui::Element load_strings(const ElfReader &static_debugger) {
  auto string_tab_content =
      ftxui::vbox({ftxui::text("strings"), ftxui::separator()});
  std::vector<std::string> strings = static_debugger.get_strings();
  for (const auto &item : strings) {
    string_tab_content = ftxui::vbox({string_tab_content, ftxui::text(item)});
  }
  return string_tab_content;
}

ftxui::Element
load_functions_arguments(const ElfRunner::RuntimeArguments runtime_value) {
  auto functions = ftxui::vbox({});
  if (runtime_value.empty())
    return functions | ftxui::border | ftxui::center;

  for (const auto &[function_name, argument] : runtime_value) {
    std::ostringstream functions_info;
    functions_info << function_name << "(" << std::get<0>(argument) << ", "
                   << std::get<1>(argument) << ", " << std::get<2>(argument)
                   << ") ";
    functions = ftxui::vbox({functions, ftxui::text(functions_info.str())});
  }
  return functions;
}
} // namespace Loader
