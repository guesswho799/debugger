#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "dependency.hpp"
#include "app_state.hpp"
#include "ui/code.hpp"
#include "ui/file.hpp"
#include "ui/function.hpp"
#include "ui/string.hpp"
#include "ui/multi_trace.hpp"
#include "ui/single_trace.hpp"


class TabController : Dependency<AppState, Code, File, Function, String, MultiTrace, SingleTrace>
{
    using Dependency::Dependency;
private:
    typedef struct State
    {
	std::vector<std::string> tab_values;
	std::vector<std::string> display_values;
	std::vector<std::string> execute_values;
        ftxui::Component tab_toggle;
	ftxui::Component display_toggle;
        ftxui::Component execute_toggle;
        ftxui::Component display_main;
        ftxui::Component execute_main;
        ftxui::Component tab_container;
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

