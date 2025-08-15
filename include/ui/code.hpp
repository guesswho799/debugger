#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

class Code : Dependency<AppState> {
  using Dependency::Dependency;

public:
  ftxui::Component get_logic();
  ftxui::Element get_code();
  void update_code();
  uint64_t get_selector();
  void reset_selector();
  void inc_selector();
  void dec_selector();

private:
  struct State {
    uint64_t selector;
    ftxui::Element disassembled_code;
  };

private:
  State _generate_initial_state();
  ftxui::Component _generate_logic();

private:
  State _state = _generate_initial_state();
  ftxui::Component _logic = _generate_logic();
};
