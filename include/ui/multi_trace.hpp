#pragma once

#include "app_state.hpp"
#include "dependency.hpp"
#include <ftxui/component/component.hpp>

class MultiTrace : Dependency<AppState> {
  using Dependency::Dependency;

public:
  ftxui::Component get_logic();

private:
  ftxui::Component _generate_logic();

private:
  ftxui::Component _logic = _generate_logic();
};
