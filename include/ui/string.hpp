#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

class String : Dependency<AppState> {
  using Dependency::Dependency;

private:
  typedef struct State {
    float scroll_x;
    float scroll_y;
    ftxui::SliderOption<float> option_x;
    ftxui::SliderOption<float> option_y;
    ftxui::Component scrollbar_x;
    ftxui::Component scrollbar_y;
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
