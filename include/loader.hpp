#pragma once

#include <vector>
#include <sstream>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"


namespace Loader
{
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
            if (line.opcodes[i] < 16) { opcodes_stream << '0'; }
            opcodes_stream << std::hex << line.opcodes[i] << ' ';
        }
        if ((line.opcodes.end() - line.opcodes.begin()) % opcodes_per_line != 0)
        {
            for (unsigned int i = 0; i < (opcodes_per_line - ((line.opcodes.end() - line.opcodes.begin()) % opcodes_per_line)) * opcodes_per_line; i++)
            {
                opcodes_stream << ' ';
            }
        }
        return ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
    }

    std::vector<ftxui::Element> load_code_lines(const std::string& function_name, const std::vector<Disassembler::Line>& assembly, std::optional<ElfRunner::runtime_mapping> runtime_data={})
    {
       std::vector<ftxui::Element> lines;
       for (const auto& line: assembly)
       {
           std::ostringstream address;
           address << std::hex << line.address;
           auto content = ftxui::hbox({
               ftxui::text("0x"),
               ftxui::text(address.str()),
               ftxui::separator(),
               get_opcodes(line),
               ftxui::separator(),
               ftxui::text(line.instruction + " " + line.arguments)
       	   });
           if (runtime_data.has_value() and runtime_data.value()[line.address] != 0)
           {
               content |= ftxui::bgcolor(ftxui::Color::Green);
           }

	   lines.push_back(content);
       }
       return lines;
    }

    ftxui::Element load_code_blocks(const std::string& function_name, const ElfReader& static_debugger, std::optional<ElfRunner::runtime_mapping> runtime_data={})
    {
       std::vector<Disassembler::Line> assembly = static_debugger.get_function_code_by_name(function_name);
       std::vector<ftxui::Element> lines        = load_code_lines(function_name, assembly, runtime_data);

       auto block = ftxui::vbox({ftxui::bold(ftxui::text(function_name)), ftxui::separator()});

       for (std::tuple<Disassembler::Line&, ftxui::Element&> item: std::views::zip(assembly, lines))
       {
	   
	   if (std::get<0>(item).is_jump) block = ftxui::vbox(block, std::get<1>(item), ftxui::separator());
	   else block = ftxui::vbox(block, std::get<1>(item));
       }
       return block;
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
}
