#pragma once

#include <vector>
#include <sstream>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"


namespace Loader
{
    std::string convert_to_hex(uint64_t number)
    {
        std::ostringstream stream;
        stream << "0x" << std::hex << number;
	return stream.str();
    }

    ftxui::Element get_opcodes(Disassembler::Line line)
    {
        auto opcodes_as_box = ftxui::vbox({});
        std::ostringstream opcodes_stream;
        const int opcodes_per_line = 3;
        for (unsigned int i = 0; i < line.opcodes.size(); i++)
        {
            if (i % opcodes_per_line == 0 && i != 0)
            {
                opcodes_as_box = ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
                opcodes_stream.str("");
                opcodes_stream.clear();
            }
            if (line.opcodes[i] < 16) opcodes_stream << '0'; 
	    opcodes_stream << std::hex << line.opcodes[i];
            if ((i+1) % opcodes_per_line != 0 or i == 0) opcodes_stream << ' ';
        }
        if ((line.opcodes.end() - line.opcodes.begin()) % opcodes_per_line != 0)
        {
            for (unsigned int i = 1; i < (opcodes_per_line - ((line.opcodes.end() - line.opcodes.begin()) % opcodes_per_line)) * opcodes_per_line; i++)
            {
                opcodes_stream << ' ';
            }
        }
        return ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
    }

    ftxui::Element load_instructions(const std::string& function_name, const std::vector<Disassembler::Line>& assembly, std::optional<uint64_t> line_selected={})
    {
        std::vector<ftxui::Element> lines;
        for (const auto& line: assembly)
        {
            std::ostringstream address;
            address << std::hex << line.address;
	    auto content = ftxui::hbox({
		ftxui::text("0x"), ftxui::text(address.str()),
		ftxui::separator(),
		get_opcodes(line),
		ftxui::separator(),
		ftxui::text(line.instruction + " " + line.arguments)
	    });
	    if (line_selected.has_value() and line_selected.value() == line.address) content = content | ftxui::inverted;
  	    lines.push_back(content);
        }

        auto block = ftxui::vbox({ftxui::hbox({ftxui::text("Instructions"), ftxui::separator(), ftxui::text(function_name) | ftxui::bold}), ftxui::separator()});
        for (std::tuple<const Disassembler::Line&, const ftxui::Element&> item: std::views::zip(assembly, lines))
        {
	    // TODO:: display jumps as arrows
	    // if (std::get<0>(item).is_jump) block = ftxui::vbox(block, std::get<1>(item), ftxui::separator());
	    // else block = ftxui::vbox(block, std::get<1>(item));
	    block = ftxui::vbox(block, std::get<1>(item));
        }
        return block | ftxui::border | ftxui::center;
    }

    ftxui::Element load_register_window(const struct user_regs_struct& registers)
    {
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
	    {"rip ", convert_to_hex(registers.rip)}
	};
	auto table = ftxui::Table(table_content);
	table.SelectColumn(0).Decorate(ftxui::color(ftxui::Color::White));
	table.SelectColumn(1).Decorate(ftxui::color(ftxui::Color::Grey50));

	return ftxui::vbox({ftxui::text("Registers") | ftxui::bold, ftxui::separator(), table.Render()}) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 25) | ftxui::border;
    }

    ftxui::Element load_trace_player(const ElfRunner::runtime_mapping& runtime_data, uint64_t code_selector)
    {
	const float data_size = runtime_data.end() - runtime_data.begin() - 1;
	return ftxui::vbox({
	    ftxui::text("Player"),
	    ftxui::separator(),
	    ftxui::gauge(code_selector / data_size) | ftxui::color(ftxui::Color::White),
	    ftxui::text("<-- " + std::to_string(code_selector) + "/" + std::to_string(static_cast<uint64_t>(data_size)) + " -->") | ftxui::center
	}) | ftxui::flex | ftxui::border;
    }

    std::vector<std::vector<std::string>> load_function_table(const ElfReader& static_debugger)
    {
       std::vector<std::vector<std::string>> function_table_info;
       for (const auto& item: static_debugger.get_functions())
       {
           function_table_info.push_back({item.name, std::to_string(item.value), std::to_string(item.size)});
       }
       return function_table_info;
    }

    ftxui::Element load_strings(const ElfReader& static_debugger)
    {
        auto string_tab_content = ftxui::vbox({ftxui::text("strings"), ftxui::separator()});
        std::vector<std::string> strings = static_debugger.get_strings();
        for (const auto& item: strings)
        {
             string_tab_content = ftxui::vbox({string_tab_content, ftxui::text(item)});
        }
        return string_tab_content;
    }
    
    ftxui::Element load_functions_arguments(const ElfRunner::runtime_arguments runtime_value)
    {
	auto functions = ftxui::vbox({});
	if (runtime_value.empty()) return functions | ftxui::border | ftxui::center;
	
	for (const auto& [function_name, argument]: runtime_value)
	{
	    std::ostringstream functions_info;
	    functions_info << function_name << "(" << std::get<0>(argument) << ", " << std::get<1>(argument) << ", " << std::get<2>(argument) << ") ";
	    functions = ftxui::vbox({functions, ftxui::text(functions_info.str())});
	}
	return functions | ftxui::border | ftxui::center;
    }
}

