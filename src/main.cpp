#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

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


std::string pick_file_menu()
{
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    std::string file_path;
    int selector = 0;
    bool should_stay = true;

    std::vector<ftxui::Component> files;
    while (should_stay)
    {
	files.clear();
	std::string search_path = file_path;
	if (!std::filesystem::exists(file_path))
	{
	    std::size_t last_directory_index = file_path.find_last_of('/');
	    if (last_directory_index == std::string::npos) { search_path = "."; }
	    else { search_path = std::string(file_path.c_str(), last_directory_index); }
	}
	for (const auto& file: std::filesystem::recursive_directory_iterator(search_path))
	{
	    files.emplace_back(ftxui::MenuEntry(file.path().string()));
	}

	ftxui::Component file_input = ftxui::Input(&file_path, "");
	auto component = ftxui::Container::Vertical({file_input});
	auto menu = ftxui::Container::Vertical(files, &selector);

	auto renderer = ftxui::Renderer(component, [&]{
	    return ftxui::vbox({
	        menu->Render(),
	        ftxui::separator(),
	        ftxui::hbox(ftxui::text(" File path : "), file_input->Render()),
	    }) | ftxui::border;
	// should exit
	}) | ftxui::CatchEvent([&](ftxui::Event event) {
	    if (event == ftxui::Event::Return)
	    {
	        should_stay = false;
	        screen.ExitLoopClosure()();
	        return true;
	    }
	    return false;
	// update selector
	}) | ftxui::CatchEvent([&](ftxui::Event event) {
	    if (event == ftxui::Event::ArrowUp) { selector++; }
	    if (event == ftxui::Event::ArrowDown) { selector--; }
	    return false;
	// update menu options
	}) | ftxui::CatchEvent([&](ftxui::Event event) {
	    if (event.is_character())
	    {
	        selector = 0;
	        file_path += event.character();
	        screen.ExitLoopClosure()();
	        return true;
	    }
	    return false;
	});

	screen.Loop(renderer);
    }

    // return files[selector];
    return "";
}

ftxui::Element gen_code_blocks(const std::string& function_name, const std::vector<Disassebler::Line>& assembly, std::optional<ElfRunner::runtime_mapping> runtime_data={})
{
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

int main(int argc, char *argv[])
{
    // TODO: FIX open file menu
    if (argc < 2) { return 1; }

    try
    {
        const std::string program_name{argv[1]};
        const std::string function_name{argv[2]};
        ElfReader static_debugger{program_name};
        ElfRunner dynamic_debugger{program_name};

        const auto func = static_debugger.get_function(function_name);
        const auto section = static_debugger.get_section(func.section_index);

	const auto assembly = static_debugger.get_function_code(func);
	auto disassembled_code = gen_code_blocks(function_name, assembly);

	std::vector<NamedSymbol> functions = static_debugger.get_functions();
	std::vector<std::vector<std::string>> function_table_info;
	function_table_info.push_back({"Name", "Address ", "Size"});
	for (const auto& item: functions)
	{
	    function_table_info.push_back({item.name, std::to_string(item.value), std::to_string(item.size)});
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

	int tab_selected = 0;
	auto code_tab        = ftxui::Renderer([&] { return disassembled_code | ftxui::border | ftxui::center; } );
	auto open_file_tab   = ftxui::Renderer([&] { return ftxui::text("in open file tab") | ftxui::border | ftxui::center; } );
	auto function_tab    = ftxui::Renderer([&] {
	    auto table = ftxui::Table(function_table_info);
	    table.SelectRow(0).Decorate(ftxui::bold);
	    table.SelectColumn(0).Decorate(ftxui::flex);
	    auto content = table.SelectRows(1, -1);
	    content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Default), 2, 0);
	    content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Grey11), 2, 1);
	    return table.Render() | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex;
	});

	function_tab         = ftxui::Container::Vertical({ftxui::Container::Horizontal({function_tab, scrollbar_y}) | ftxui::flex, scrollbar_x});
	auto string_tab      = ftxui::Renderer([&] { return ftxui::text("in strings tab") | ftxui::border | ftxui::center; } );
	auto run_program_tab = ftxui::Renderer([&] { 
	    std::variant<ElfRunner::runtime_mapping, ElfRunner::current_address> runtime_value = dynamic_debugger.run_function(func.value, func.size);
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
	        disassembled_code = gen_code_blocks(function_name, assembly, std::get<ElfRunner::runtime_mapping>(runtime_value));
		screen.PostEvent(ftxui::Event::Character('m'));
		return ftxui::vbox({});
	    }
	});
	auto middle          = ftxui::Container::Tab({code_tab, open_file_tab, function_tab, string_tab, run_program_tab}, &tab_selected);

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
	            disassembled_code = gen_code_blocks(function_name, assembly);
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
        std::cout << "cought exception: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}
