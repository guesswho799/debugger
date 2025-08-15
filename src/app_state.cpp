#include "app_state.hpp"
#include <cstdint>

void AppState::backspace() { screen.PostEvent(ftxui::Event::Backspace); }

void AppState::refresh_screen() {
  screen.PostEvent(ftxui::Event::Character('m'));
}

void AppState::update_binary(const std::string &_binary_path) {
  static_debugger = std::optional<ElfReader>(_binary_path);
  dynamic_debugger = std::optional<ElfRunner>(_binary_path);
  binary_path = _binary_path;
  function_name = "_start";
  refresh_screen();
}

void AppState::update_function(const std::string &_function_name) {
  if (dynamic_debugger.has_value()) {
    dynamic_debugger->reset();
  }
  function_name = _function_name;
  refresh_screen();
}

void AppState::run_function() {
  const bool is_pie = static_debugger->is_position_independent();
  const uint64_t base_address = dynamic_debugger->get_base_address(is_pie);
  const auto function =
      static_debugger->get_function(function_name, base_address);
  const auto function_calls =
      static_debugger->get_function_calls(function_name, base_address);

  dynamic_debugger->run_function(function, function_calls, base_address);
}

void AppState::run_all_functions() {
  const bool is_pie = static_debugger->is_position_independent();
  const uint64_t base_address = dynamic_debugger->get_base_address(is_pie);
  const auto functions =
      static_debugger->get_implemented_functions(base_address);
  dynamic_debugger->run_functions(functions, base_address);
}
