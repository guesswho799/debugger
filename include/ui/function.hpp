#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include "ui/code.hpp"

class Function : Dependency<AppState, Code> {
  using Dependency::Dependency;

private:
  struct State {
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
  };

public:
  ftxui::Component get_logic();

private:
  State _generate_initial_state();
  ftxui::Component _generate_logic();

  void _store_functions_from_input();

private:
  State _state = _generate_initial_state();
  ftxui::Component _logic = _generate_logic();
};
