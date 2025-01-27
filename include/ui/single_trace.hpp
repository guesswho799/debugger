#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include "ui/code.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

class SingleTrace : Dependency<AppState, Code> {
  using Dependency::Dependency;

private:
  typedef struct State {
    bool view_result;
    uint64_t selector;
  } State_t;

public:
  ftxui::Component get_logic();
  void enable_result();
  void disable_result();
  uint64_t get_selector();
  void inc_selector();
  void dec_selector();
  void reset_selector();

private:
  State_t _generate_initial_state();
  ftxui::Component _generate_logic();

private:
  State_t _state = _generate_initial_state();
  ftxui::Component _logic = _generate_logic();
};
