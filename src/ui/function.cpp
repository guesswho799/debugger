#include "ui/function.hpp"
#include "loader.hpp"


ftxui::Component Function::get_logic()
{
    return _logic;
}

Function::State_t Function::_generate_initial_state()
{
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
    return {
        .scroll_x = scroll_x,
        .scroll_y = scroll_y,
        .option_x = option_x,
        .option_y = option_y,
        .scrollbar_x = scrollbar_x,
        .scrollbar_y = scrollbar_y,
	.function_input_content = "",
	.function_input = ftxui::Input(&_state.function_input_content, ""),
	.function_selector = 1,
	.function_table_content = {}
    };
}

ftxui::Component Function::_generate_logic()
{
    auto function_tab    = ftxui::Renderer(_state.function_input, [&] {
        _state.function_table_content.clear();
        if (_state.function_input_content.empty() or !get<AppState>()->in_search_mode)
        {
	    _state.function_table_content = Loader::load_function_table(get<AppState>()->static_debugger.value());
	}
        else
        {
    	    for (const auto& item: Loader::load_function_table(get<AppState>()->static_debugger.value()))
    	        if (item[0].find(_state.function_input_content) != std::string::npos)
    	            _state.function_table_content.push_back(item);
        }
        _state.function_table_content.insert(_state.function_table_content.begin(), 1, {"Name", "Address ", "Size"});

        auto table = ftxui::Table(_state.function_table_content);
        table.SelectRow(0).Decorate(ftxui::bold);
        table.SelectColumn(0).Decorate(ftxui::flex);

        if (get<AppState>()->in_search_mode)
        {
            table.SelectRow(_state.function_selector).Decorate(ftxui::bgcolor(ftxui::Color::Grey11));
            return ftxui::vbox({
                ftxui::window(ftxui::text("Search function"), _state.function_input->Render()),
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
                ftxui::window(ftxui::text("Functions"), table.Render() | ftxui::focusPositionRelative(_state.scroll_x, _state.scroll_y) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex)
    	}) | ftxui::flex | ftxui::border;
        }
    }) | ftxui::CatchEvent([&](ftxui::Event event)
    {
    	if (event == ftxui::Event::Escape)
	{
	    get<AppState>()->in_search_mode = false; 
	}
    	if (event == ftxui::Event::Character('/') and get<AppState>()->in_search_mode == false)
    	{
	    get<AppState>()->in_search_mode = true;
	    _state.function_input_content = "";
	    get<AppState>()->backspace();
	}
    	if (get<AppState>()->in_search_mode)
    	{
            if (event == ftxui::Event::ArrowUp or event == ftxui::Event::TabReverse)
    	    {
    	        _state.function_selector--;
                if (_state.function_selector < 1) _state.function_selector = 1;
    		return true;
    	    }
            if (event == ftxui::Event::ArrowDown or event == ftxui::Event::Tab)
    	    {
    	        _state.function_selector++;
		const auto table_size = _state.function_table_content.end() - _state.function_table_content.begin() - 1;
                if (_state.function_selector > table_size) _state.function_selector = table_size;
    		return true;
    	    }
            if (event == ftxui::Event::Return)
            {
		const auto function_name = _state.function_table_content[_state.function_selector][0];
		get<AppState>()->update_function(function_name);
    		get<Code>()->update_code();
                get<AppState>()->in_search_mode = false;
                _state.function_input_content = "";
    		_state.function_selector = 1;
            }
        }
    	return false;
    });

    return ftxui::Container::Vertical({ftxui::Container::Horizontal({function_tab, _state.scrollbar_y}) | ftxui::flex, _state.scrollbar_x});
}

