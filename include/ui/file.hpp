#pragma once

#include "loader.hpp"
#include "code.hpp"


class File : Dependency<AppState, Code>
{
    using Dependency::Dependency;

private:
    typedef struct State
    {
	std::string file_path;
	ftxui::Component file_input;
	std::vector<ftxui::Component> files;
	std::vector<std::string> files_as_strings;
	int open_file_selector;
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

