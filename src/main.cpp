#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/util/ref.hpp>

#include "disassembler.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include "status.hpp"

#define CHECK_LOADED_ELF() if (!static_debugger.has_value() or !dynamic_debugger.has_value()) return ftxui::text("no file loaded..")


ftxui::Element load_code_blocks(const std::string& function_name, const ElfReader& static_debugger, std::optional<ElfRunner::runtime_mapping> runtime_data={})
{
    auto assembly = static_debugger.get_function_code_by_name(function_name);
    auto disassembled_code = ftxui::vbox({ftxui::bold(ftxui::text(function_name)), ftxui::separator()});
    for (const auto& item: assembly)
    {
        std::ostringstream address;
        address << std::hex << item.address;
        std::ostringstream opcodes_stream;
        auto opcodes_as_box = ftxui::vbox({});
        const int opcodes_per_line = 3;
        for (unsigned int i = 0; i < item.opcodes.size(); i++)
        {
	    if (i % opcodes_per_line == 0 && i != 0)
	    {
	        opcodes_as_box = ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
	        opcodes_stream.str("");
	        opcodes_stream.clear();
	    }
	    if (item.opcodes[i] < 16) { opcodes_stream << '0'; }
	    opcodes_stream << std::hex << item.opcodes[i] << ' ';
	}
	if ((item.opcodes.end() - item.opcodes.begin()) % opcodes_per_line != 0)
	{
	    for (unsigned int i = 0; i < (opcodes_per_line - ((item.opcodes.end() - item.opcodes.begin()) % opcodes_per_line)) * opcodes_per_line; i++)
	    {
	        opcodes_stream << ' ';
	    }
        }
        opcodes_as_box = ftxui::vbox({opcodes_as_box, ftxui::text(opcodes_stream.str())});
	auto content = ftxui::hbox({
	    ftxui::text("0x"),
	    ftxui::text(address.str()),
	    ftxui::separator(),
	    opcodes_as_box,
	    ftxui::separator(),
	    ftxui::text(item.disassemble)
    	});
	if (runtime_data.has_value() and runtime_data.value()[item.address] != 0)
	{
	    content |= ftxui::bgcolor(ftxui::Color::Green);
	}

        disassembled_code = ftxui::vbox(disassembled_code, content);
    }
    return disassembled_code;
}

std::vector<std::vector<std::string>> load_function_table(const ElfReader& static_debugger)
{
    std::vector<std::vector<std::string>> function_table_info;
    function_table_info.push_back({"Name", "Address ", "Size"});
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


int main(int argc, char *argv[])
{
    int tab_selected = 0;
    bool in_search_mode = false;
    std::string binary_path;
    std::string function_name;
    if (argc < 2) { tab_selected = 1; in_search_mode = true; }
    if (argc >= 2) { binary_path = argv[1]; }
    if (argc < 3) { function_name = "_start"; }
    if (argc >= 3) { function_name = argv[2]; }

    try
    {
	std::optional<ElfReader> static_debugger;
        std::optional<ElfRunner> dynamic_debugger;
	auto disassembled_code = ftxui::text("no file loaded..");
	if (!binary_path.empty())
	{
            static_debugger = std::optional<ElfReader>(binary_path);
            dynamic_debugger = std::optional<ElfRunner>(binary_path);
	    disassembled_code = load_code_blocks(function_name, static_debugger.value());
	}

	auto screen = ftxui::ScreenInteractive::Fullscreen();
	// TODO: fix slider acting wierd
	float scroll_x = 0.f;
	float scroll_y = 0.f;
	ftxui::SliderOption<float> option_x;
	option_x.value = &scroll_x;
	option_x.min = 0.f;
	option_x.max = 1.f;
	option_x.increment = 0.5f;
	option_x.direction = ftxui::Direction::Right;
	option_x.color_active = ftxui::Color::Black;
	option_x.color_inactive = ftxui::Color::Black;
	auto scrollbar_x = ftxui::Slider(option_x);
	ftxui::SliderOption<float> option_y;
	option_y.value = &scroll_y;
	option_y.min = 0.f;
	option_y.max = 1.f;
	option_y.increment = 0.01f;
	option_y.direction = ftxui::Direction::Down;
	option_y.color_active = ftxui::Color::Black;
	option_y.color_inactive = ftxui::Color::Black;
	auto scrollbar_y = ftxui::Slider(option_y);

	auto code_tab = ftxui::Renderer([&] {
	    CHECK_LOADED_ELF();
	    return disassembled_code | ftxui::border | ftxui::center;
	} );

	std::string file_path;
	std::vector<ftxui::Component> files;
	std::vector<std::string> files_as_strings;
	ftxui::Component file_input = ftxui::Input(&file_path, "");
	int open_file_selector = 0;
	auto open_file_tab   = ftxui::Renderer(file_input, [&] {
            files.clear();
            files_as_strings.clear();
            std::string search_path = file_path;
            if (!std::filesystem::exists(file_path))
            {
                std::size_t last_directory_index = file_path.find_last_of('/');
                if (last_directory_index == std::string::npos) { search_path = "."; }
                else { search_path = std::string(file_path.c_str(), last_directory_index); }
            }
	    if (std::filesystem::is_directory(search_path))
	    {
                int i = 0;
                for (const auto& file: std::filesystem::recursive_directory_iterator(search_path))
                {
                    files.emplace_back(ftxui::MenuEntry(file.path().string()));
                    files_as_strings.emplace_back(file.path().string());
                    if (++i == 20) break;
                }
	    }
	    else
	    {
                files.emplace_back(ftxui::MenuEntry(search_path));
	        files_as_strings.emplace_back(search_path);
	    }
            auto menu = ftxui::Container::Vertical(files, &open_file_selector);
	    if (in_search_mode)
	    {
                return ftxui::vbox({
                    ftxui::hbox(ftxui::text(" File path : "), file_input->Render()),
                    ftxui::separator(),
                    menu->Render(),
                }) | ftxui::border;
	    }
	    else
	    {
                return ftxui::vbox({
		    ftxui::text("press / to start searching..."),
                    ftxui::separator(),
		    menu->Render()
		}) | ftxui::border;
	    }
	}) | ftxui::CatchEvent([&](ftxui::Event event)
	    {
		if (event == ftxui::Event::Escape         and in_search_mode == true) in_search_mode = false; 
		if (event == ftxui::Event::Character('/') and in_search_mode == false)
		{ in_search_mode = true; file_path = ""; screen.PostEvent(ftxui::Event::Backspace); }
		if (in_search_mode)
		{
	            if (event == ftxui::Event::ArrowUp or event == ftxui::Event::TabReverse) open_file_selector--;
	            if (event == ftxui::Event::ArrowDown or event == ftxui::Event::Tab) open_file_selector++;
	            if (open_file_selector < 0)                               open_file_selector = 0;
	            if (open_file_selector > files.end() - files.begin() - 1) open_file_selector = files.end() - files.begin() - 1;
	            if (event == ftxui::Event::Return)
	            {
	                binary_path = files_as_strings[open_file_selector];
	                static_debugger = std::optional<ElfReader>(binary_path);
	                dynamic_debugger = std::optional<ElfRunner>(binary_path);
	                disassembled_code = load_code_blocks(function_name, static_debugger.value());
	                in_search_mode = false;
	                file_path = "";
	                screen.PostEvent(ftxui::Event::Character('m'));
	            }
	        }
		return false;
	    });
	auto function_tab    = ftxui::Renderer([&] {
	    CHECK_LOADED_ELF();
	    auto table = ftxui::Table(load_function_table(static_debugger.value()));
	    table.SelectRow(0).Decorate(ftxui::bold);
	    table.SelectColumn(0).Decorate(ftxui::flex);
	    auto content = table.SelectRows(1, -1);
	    content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Default), 2, 0);
	    content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Grey11), 2, 1);
	    return table.Render() | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex;
	});

	function_tab         = ftxui::Container::Vertical({ftxui::Container::Horizontal({function_tab, scrollbar_y}) | ftxui::flex, scrollbar_x});
	auto string_tab      = ftxui::Renderer([&] {
	    CHECK_LOADED_ELF();
	    return load_strings(static_debugger.value()) | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex;
	});
	string_tab           = ftxui::Container::Horizontal({string_tab, scrollbar_y}) | ftxui::flex;
	auto run_program_tab = ftxui::Renderer([&] { 
	    CHECK_LOADED_ELF();
	    const auto func = static_debugger.value().get_function(function_name);
	    const std::variant<ElfRunner::runtime_mapping, ElfRunner::current_address> runtime_value = dynamic_debugger.value().run_function(func.value, func.size);
	    if (std::holds_alternative<ElfRunner::current_address>(runtime_value))
	    {
	        std::ostringstream function_address;
	        function_address << "function address: 0x" << std::hex << std::to_string(func.value);
	        std::ostringstream current_information;
	        current_information << "current address: 0x" << std::hex << std::to_string(std::get<ElfRunner::current_address>(runtime_value));
		screen.PostEvent(ftxui::Event::Character('a')); // TODO: there is got to be a better way to trigger a screen redraw
	        return ftxui::vbox({ftxui::text("Running program..."), ftxui::text(function_address.str()), ftxui::text(current_information.str())}) | ftxui::border | ftxui::center;
	    }
	    else
	    {
	        disassembled_code = load_code_blocks(function_name, static_debugger.value(), std::get<ElfRunner::runtime_mapping>(runtime_value));
		screen.PostEvent(ftxui::Event::Character('m'));
		return ftxui::vbox({});
	    }
	});
	auto middle = ftxui::Container::Tab({code_tab, open_file_tab, function_tab, string_tab, run_program_tab}, &tab_selected);

	std::vector<std::string> tab_values = {"Main", "Open file", "Functions", "Strings", "Debug function", "Quit"};
	auto option = ftxui::MenuOption::HorizontalAnimated();
	option.elements_infix = [] { return ftxui::text(" | "); };
	option.entries_option.transform = [](const ftxui::EntryState& state) {
		ftxui::Element e = ftxui::text(state.label) | ftxui::color(ftxui::Color::Blue);
		if (state.active)
		    e = e | ftxui::bold | ftxui::underlined;
		return e;
	};
	auto tab_toggle = ftxui::Menu(&tab_values, &tab_selected, option);
	auto top        = ftxui::Renderer([&] { return ftxui::hbox({ftxui::text(" "), tab_toggle->Render()}) | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1); });

	auto renderer = ftxui::Renderer(middle, [&] { return ftxui::vbox({top->Render(), ftxui::separator(), middle->Render()}); })
	    | ftxui::CatchEvent([&](ftxui::Event event)
	    {
		if (in_search_mode) return false;
		if (event == ftxui::Event::Character('m'))
		{
	            tab_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('o'))
		{
	            tab_selected = 1;
	    	}
		else if (event == ftxui::Event::Character('f'))
		{
	            tab_selected = 2;
	    	}
		else if (event == ftxui::Event::Character('s'))
		{
	            tab_selected = 3;
	    	}
		else if (event == ftxui::Event::Character('d'))
		{
	            tab_selected = 4;
	    	}
		else if (event == ftxui::Event::Character('q'))
		{
	            screen.ExitLoopClosure()();
	            return true;
	    	}
		return false;
	});
	screen.SetCursor(ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));
	screen.Loop(renderer);
    }
    catch (const std::exception& exception)
    {
        std::cout << "cought exception: " << exception.what() << std::endl << binary_path << " " << function_name << std::endl;
        return 1;
    }

    return 0;
}
