#pragma once

#include "dependency.hpp"
#include "app_state.hpp"
#include "ui/code.hpp"


class Function : Dependency<AppState, Code>
{
    using Dependency::Dependency;
private:
    typedef struct State
    {
        float scroll_x;
        float scroll_y;
        ftxui::SliderOption<float> option_x;
        ftxui::SliderOption<float> option_y;
        ftxui::Component scrollbar_x;
        ftxui::Component scrollbar_y;
	std::string function_input_content;
	ftxui::Component function_input;
	int function_selector;
	std::vector<std::vector<std::string>> function_table_content;
    } State_t;

public:
    ftxui::Component get_logic();

private:
    State_t _generate_initial_state();
    ftxui::Component _generate_logic();

private:
    State_t _state = _generate_initial_state();
    ftxui::Component _logic = _generate_logic();
};

