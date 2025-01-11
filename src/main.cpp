
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ranges>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/util/ref.hpp>

#include "loader.hpp"
#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include "status.hpp"

#define CHECK_LOADED_ELF() if (!static_debugger.has_value() or !dynamic_debugger.has_value()) return ftxui::text("no file loaded..")



int main(int argc, char *argv[])
{
    int display_selected = 0;
    bool in_search_mode = false;
    std::string binary_path;
    std::string function_name;
    if (argc < 2) { display_selected = 1; in_search_mode = true; }
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
            static_debugger   = std::optional<ElfReader>(binary_path);
            dynamic_debugger  = std::optional<ElfRunner>(binary_path);
	    disassembled_code = Loader::load_instructions(function_name, static_debugger.value().get_function_code_by_name(function_name));
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
	    return disassembled_code;
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
                    ftxui::window(ftxui::text("Search file"), file_input->Render()),
                    ftxui::separator(),
                    ftxui::window(ftxui::text("Files"), menu->Render())
                }) | ftxui::border;
	    }
	    else
	    {
                return ftxui::vbox({
		    ftxui::text(" Press / to start searching..."),
                    ftxui::separator(),
                    ftxui::window(ftxui::text("Files"), menu->Render())
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
	    		disassembled_code = Loader::load_instructions(function_name, static_debugger.value().get_function_code_by_name(function_name));
	                in_search_mode = false;
	                file_path = "";
	                screen.PostEvent(ftxui::Event::Character('m'));
	            }
	        }
		return false;
	    });

	std::string function_input_content;
	ftxui::Component function_input = ftxui::Input(&function_input_content, "");
	int function_selector = 1;
	std::vector<std::vector<std::string>> function_table_content;
	auto function_tab    = ftxui::Renderer(function_input, [&] {
	    CHECK_LOADED_ELF();
	    if (function_input_content.empty() or !in_search_mode)
	    { function_table_content = Loader::load_function_table(static_debugger.value()); }
	    else
	    {
	        function_table_content = Loader::load_function_table(static_debugger.value()) |
		    std::views::filter([&](const auto& item) { return item[0].find(function_input_content) != std::string::npos; }) |
		    std::ranges::to<std::vector>();
	    }
	    function_table_content.insert(function_table_content.begin(), 1, {"Name", "Address ", "Size"});
	    auto table = ftxui::Table(function_table_content);
	    table.SelectRow(0).Decorate(ftxui::bold);
	    table.SelectColumn(0).Decorate(ftxui::flex);
	    if (in_search_mode)
	    {
	        table.SelectRow(function_selector).Decorate(ftxui::bgcolor(ftxui::Color::Grey11));
                return ftxui::vbox({
                    ftxui::window(ftxui::text("Search function"), function_input->Render()),
                    ftxui::separator(),
                    ftxui::window(ftxui::text("Functions"), table.Render() | ftxui::frame | ftxui::flex)
                }) | ftxui::flex | ftxui::border;
	    }
	    else
	    {
	        auto content = table.SelectRows(1, -1);
	        content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Default), 2, 0);
	        content.DecorateCellsAlternateRow(ftxui::bgcolor(ftxui::Color::Grey11), 2, 1);
                return ftxui::vbox({
		    ftxui::text(" Press / to start searching..."),
                    ftxui::separator(),
                    ftxui::window(ftxui::text("Functions"), table.Render() | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex)
		}) | ftxui::flex | ftxui::border;
	    }
	}) | ftxui::CatchEvent([&](ftxui::Event event)
	    {
		if (event == ftxui::Event::Escape         and in_search_mode == true) in_search_mode = false; 
		if (event == ftxui::Event::Character('/') and in_search_mode == false)
		{ in_search_mode = true; function_input_content = ""; screen.PostEvent(ftxui::Event::Backspace); }
		if (in_search_mode)
		{
	            if (event == ftxui::Event::ArrowUp or event == ftxui::Event::TabReverse)
		    {
		        function_selector--;
	                if (function_selector < 1) function_selector = 1;
			return true;
		    }
	            if (event == ftxui::Event::ArrowDown or event == ftxui::Event::Tab)
		    {
		        function_selector++;
	                if (function_selector > function_table_content.end() - function_table_content.begin() - 1) function_selector = function_table_content.end() - function_table_content.begin() - 1;
			return true;
		    }
	            if (event == ftxui::Event::Return)
	            {
			if (dynamic_debugger.has_value()) dynamic_debugger->reset();
	                function_name = function_table_content[function_selector][0];
	    		disassembled_code = Loader::load_instructions(function_name, static_debugger.value().get_function_code_by_name(function_name));
	                in_search_mode = false;
	                function_input_content = "";
			function_selector = 0;
	                screen.PostEvent(ftxui::Event::Character('m'));
	            }
	        }
		return false;
	    });

	function_tab         = ftxui::Container::Vertical({ftxui::Container::Horizontal({function_tab, scrollbar_y}) | ftxui::flex, scrollbar_x});
	auto string_tab      = ftxui::Renderer([&] {
	    CHECK_LOADED_ELF();
	    return Loader::load_strings(static_debugger.value()) | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex;
	});
	string_tab           = ftxui::Container::Horizontal({string_tab, scrollbar_y}) | ftxui::flex;
	auto trace_functions_tab = ftxui::Renderer([&] { 
	    CHECK_LOADED_ELF();
	    if (dynamic_debugger.value().is_dead())
	    {
	    	const auto runtime_value = dynamic_debugger.value().get_runtime_arguments();
	        return ftxui::vbox({ftxui::text("Program finished"), Loader::load_functions_arguments(runtime_value)}) | ftxui::border | ftxui::center;
	    }
	    screen.PostEvent(ftxui::Event::Character('p')); // TODO: there is got to be a better way to trigger a screen redraw
	    const auto functions = static_debugger.value().get_implemented_functions();
	    dynamic_debugger.value().run_functions(functions);
	    return ftxui::vbox({ftxui::text("Running process..."), Loader::load_functions_arguments(dynamic_debugger.value().get_runtime_arguments())}) | ftxui::border | ftxui::center;
	});

	bool view_result = false;
	uint64_t code_selector = 0;
	auto trace_function_tab = ftxui::Renderer([&] { 
	    CHECK_LOADED_ELF();
	    if (dynamic_debugger.value().is_dead() or view_result == true)
	    {
                const auto assembly        = static_debugger.value().get_function_code_by_name(function_name);
		const auto registers       = dynamic_debugger.value().get_runtime_mapping()[code_selector];
	        const auto code_block      = Loader::load_instructions(function_name, assembly, registers.rip);
	        const auto trace_player    = Loader::load_trace_player(dynamic_debugger.value().get_runtime_mapping(), code_selector);
	        const auto register_window = Loader::load_register_window(registers);
		return ftxui::vbox({trace_player, ftxui::hbox({code_block, register_window})}) | ftxui::center;
	    }

	    screen.PostEvent(ftxui::Event::Character('p')); // TODO: there is got to be a better way to trigger a screen redraw
	    const auto function       = static_debugger.value().get_function(function_name);
	    const auto function_calls = static_debugger.value().get_function_calls(function_name);
	    dynamic_debugger.value().run_function(function, function_calls);

            std::ostringstream function_status;
            function_status << "Selected function (" << function_name << ") ";
	    if (dynamic_debugger.value().get_runtime_mapping().empty()) function_status << "not hit yet";
	    else function_status << "hit. Press V to view results";
	    return ftxui::vbox({ftxui::text("Running process..."), ftxui::text(function_status.str())}) | ftxui::border | ftxui::center;
	});

	auto info_tab      = ftxui::Renderer([&] {
	    CHECK_LOADED_ELF();
	    return ftxui::vbox({
			    ftxui::text("Program execution tab"),
			    ftxui::separator(),
			    ftxui::text("Press A for a summary of all functions hit"),
			    ftxui::text("Press S for a detailed summary of a selected function")
			    }) | ftxui::border | ftxui::center;
	});

	std::vector<std::string> tab_values = {"Display", "Execute"};
	std::vector<std::string> display_values = {"Main", "Open file", "Functions", "Strings"};
	std::vector<std::string> execute_values = {"Information", "All functions", "Single function"};
	int tab_selected = 0;
	int execute_selected = 0;

	auto option = ftxui::MenuOption::HorizontalAnimated();
	option.elements_infix = [] { return ftxui::text(" | "); };
	option.entries_option.transform = [](const ftxui::EntryState& state) {
		ftxui::Element e = ftxui::text(state.label) | ftxui::color(ftxui::Color::Blue);
		if (state.active)
		    e = e | ftxui::bold | ftxui::underlined;
		return e;
	};
	auto display_toggle = ftxui::Menu(&display_values, &display_selected, option);
	auto execute_toggle = ftxui::Menu(&execute_values, &execute_selected, option);
	auto display_main   = ftxui::Container::Tab({code_tab, open_file_tab, function_tab, string_tab}, &display_selected);
	auto execute_main   = ftxui::Container::Tab({info_tab, trace_functions_tab, trace_function_tab}, &execute_selected);
	auto display        = ftxui::Renderer([&] { return ftxui::vbox({ftxui::hbox({ftxui::text(" "), display_toggle->Render()}) | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1), display_main->Render()}); });
	auto execute        = ftxui::Renderer([&] { return ftxui::vbox({ftxui::hbox({ftxui::text(" "), execute_toggle->Render()}) | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1), execute_main->Render()}); });

	auto toggle_option = ftxui::MenuOption::HorizontalAnimated();
	toggle_option.elements_infix = [] { return ftxui::text(" | "); };
	toggle_option.entries_option.transform = [](const ftxui::EntryState& state) {
		ftxui::Element e = ftxui::text(state.label);
		if (state.active)
		    e = e | ftxui::bold | ftxui::inverted;
		return e;
	};
	auto tab_toggle = ftxui::Menu(&tab_values, &tab_selected, toggle_option);
	auto tab_container = ftxui::Container::Tab({display, execute}, &tab_selected);

	auto container = ftxui::Container::Vertical({tab_toggle, tab_container});

	auto renderer = ftxui::Renderer(execute_main, [&] { return ftxui::vbox({
				ftxui::hbox({tab_toggle->Render() | ftxui::flex, ftxui::text("Quit")}),
				tab_container->Render()
			}); })
	    | ftxui::CatchEvent([&](ftxui::Event event)
	    {
		if (in_search_mode) return false;
		if (event == ftxui::Event::Character('d'))
		{
	            tab_selected = 0;
		    display_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('e'))
		{
	            tab_selected = 1;
		    execute_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('m'))
		{
	            display_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('o'))
		{
	            display_selected = 1;
	    	}
		else if (event == ftxui::Event::Character('f'))
		{
	            display_selected = 2;
	    	}
		else if (tab_selected == 0 and event == ftxui::Event::Character('s'))
		{
	            display_selected = 3;
	    	}
		else if (event == ftxui::Event::Character('i'))
		{
	            execute_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('a'))
		{
	            execute_selected = 1;
		    dynamic_debugger.value().reset();
	    	}
		else if (tab_selected == 1 and event == ftxui::Event::Character('s'))
		{
	            execute_selected = 2;
		    view_result = false;
		    code_selector = 0;
		    dynamic_debugger.value().reset();
		    dynamic_debugger.value().reset();
	    	}
		if (tab_selected == 1 and execute_selected == 2 and event == ftxui::Event::Character('v'))
		{
		    view_result = true;
		}
		else if (event == ftxui::Event::Character('q'))
		{
	            screen.ExitLoopClosure()();
	            return true;
	    	}
		if (tab_selected == 1 and execute_selected == 2)
		{
		    if (event == ftxui::Event::ArrowLeft)
		    {
			if (code_selector != 0) code_selector--;
			return true;
		    }
		    if (event == ftxui::Event::ArrowRight)
		    {
			code_selector++;
			const auto container = dynamic_debugger.value().get_runtime_mapping();
			const uint64_t code_size = container.end() - container.begin() - 1;
			if (code_selector > code_size) code_selector = code_size;
			return true;
		    }
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

