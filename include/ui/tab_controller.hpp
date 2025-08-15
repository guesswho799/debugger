#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include "ui/code.hpp"
#include "ui/file.hpp"
#include "ui/function.hpp"
#include "ui/multi_trace.hpp"
#include "ui/single_trace.hpp"
#include "ui/string.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

class TabController : Dependency<AppState, Code, File, Function, String,
                                 MultiTrace, SingleTrace> {
  using Dependency::Dependency;

private:
  struct State {
    std::vector<std::string> tab_values;
    std::vector<std::string> display_values;
    std::vector<std::string> execute_values;
    ftxui::Component tab_toggle;
    ftxui::Component display_toggle;
    ftxui::Component execute_toggle;
    ftxui::Component display_main;
    ftxui::Component execute_main;
    ftxui::Component tab_container;
  };

public:
  ftxui::Component get_logic();

private:
  State _generate_initial_state();
  ftxui::Component _generate_logic();

private:
  State _state = _generate_initial_state();
  ftxui::Component _logic = _generate_logic();
};
