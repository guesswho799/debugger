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


void log_function(std::vector<Disassebler::Line> lines, std::map<int, int> runtime_data)
{
    std::cout << "[counter] address: opcodes | assembly" << std::endl;
    for (const auto& line : lines)
    {
        std::cout << "[" << runtime_data[line.address] << "] ";
        std::cout << line.address << ": ";
        for (const auto& opcode: line.opcodes)
        {
            std::cout << std::hex << opcode << " ";
        }
        std::cout << line.disassemble << "\n";
    }
}

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

        const auto main = static_debugger.get_function(function_name);
        const auto section = static_debugger.get_section(main.section_index);

        // auto address_counter = dynamic_debugger.run_function(main.value, main.size);

        auto assembly = static_debugger.get_function_code(main);
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

	    disassembled_code = ftxui::vbox(disassembled_code,
		    ftxui::hbox({
			    ftxui::text("0x"),
			    ftxui::text(address.str()),
			    ftxui::separator(),
			    opcodes_as_box,
			    ftxui::separator(),
			    ftxui::text(item.disassemble)
		    }));
	}

	std::vector<NamedSymbol> functions = static_debugger.get_functions();
	auto function_table_info = ftxui::vbox({});
	for (const auto& item: functions)
	{
	    function_table_info = ftxui::vbox(function_table_info, ftxui::hbox({ftxui::text(item.name), ftxui::filler(), ftxui::separator(), ftxui::text(std::to_string(item.value)), ftxui::separator(), ftxui::text(std::to_string(item.size)), ftxui::text(" ")}), ftxui::separator());
	}

	auto screen = ftxui::ScreenInteractive::Fullscreen();
	// TODO: fix slider acting wierd
	float scroll_x = 0.f;
	float scroll_y = 0.f;
	ftxui::SliderOption<float> option_x;
	option_x.value = &scroll_x;
	option_x.min = 0.f;
	option_x.max = 1.f;
	option_x.increment = 0.1f;
	option_x.direction = ftxui::Direction::Right;
	option_x.color_active = ftxui::Color::Blue;
	option_x.color_inactive = ftxui::Color::Black;
	auto scrollbar_x = ftxui::Slider(option_x);
	ftxui::SliderOption<float> option_y;
	option_y.value = &scroll_y;
	option_y.min = 0.f;
	option_y.max = 1.f;
	option_y.increment = 0.1f;
	option_y.direction = ftxui::Direction::Down;
	option_y.color_active = ftxui::Color::Blue;
	option_y.color_inactive = ftxui::Color::Black;
	auto scrollbar_y = ftxui::Slider(option_y);

	int tab_selected = 0;
	auto code_tab = ftxui::Renderer([&] { return disassembled_code | ftxui::border | ftxui::center; } );
	auto open_file_tab = ftxui::Renderer([&] { return ftxui::text("in open file tab") | ftxui::border | ftxui::center; } );
	auto string_tab = ftxui::Renderer([&] { return ftxui::text("in strings tab") | ftxui::border | ftxui::center; } );
	auto run_program_tab = ftxui::Renderer([&] { return ftxui::text("in run program tab") | ftxui::border | ftxui::center; } );
	auto attach_to_program_tab = ftxui::Renderer([&] { return ftxui::text("in attach to program tab") | ftxui::border | ftxui::center; } );
	auto middle = ftxui::Container::Tab({code_tab, open_file_tab, string_tab, run_program_tab, attach_to_program_tab}, &tab_selected);
	auto left   = ftxui::Renderer([&] {
	    return ftxui::vbox(ftxui::text("Function table") | ftxui::center,
		   ftxui::text("Name Address Size") | ftxui::center,
		   ftxui::separator(),
		   function_table_info | ftxui::focusPositionRelative(scroll_x, scroll_y) | ftxui::frame | ftxui::flex);
	});
	left = ftxui::Container::Vertical({ftxui::Container::Horizontal({left, scrollbar_y}) | ftxui::flex, scrollbar_x});

	std::vector<std::string> tab_values = {"main", "file", "strings", "run", "attach", "quit"};
	auto option = ftxui::MenuOption::HorizontalAnimated();
	option.entries_option.transform = [](const ftxui::EntryState& state) {
		ftxui::Element e = ftxui::text(state.label) | ftxui::color(ftxui::Color::Blue);
		if (state.active)
		    e = e | ftxui::bold | ftxui::underlined;
		return e;
	};
	option.elements_infix = [] { return ftxui::text(" | "); };
	auto tab_toggle = ftxui::Menu(&tab_values, &tab_selected, option);
	auto top    = ftxui::Renderer([&] { return tab_toggle->Render(); });


	int left_size = 40;
	int top_size = 1;
	auto container = middle;
	container = ftxui::ResizableSplitLeft(left, container, &left_size);
	container = ftxui::ResizableSplitTop(top, container, &top_size);

	auto renderer = ftxui::Renderer(container, [&] { return container->Render() | ftxui::border; })
	    | ftxui::CatchEvent([&](ftxui::Event event)
	    {
		if (event == ftxui::Event::Character('m'))
		{
	            tab_selected = 0;
	    	}
		else if (event == ftxui::Event::Character('f'))
		{
	            tab_selected = 1;
	    	}
		else if (event == ftxui::Event::Character('s'))
		{
	            tab_selected = 2;
	    	}
		else if (event == ftxui::Event::Character('r'))
		{
	            tab_selected = 3;
	    	}
		else if (event == ftxui::Event::Character('a'))
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
        std::cout << "cought exception: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}
