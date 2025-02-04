#pragma once

#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include <cstdint>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

struct AppState {
  int tab_selected;
  int display_selected;
  int execute_selected;
  bool in_search_mode;
  std::string binary_path;
  std::string function_name;
  std::optional<ElfReader> static_debugger;
  std::optional<ElfRunner> dynamic_debugger;
  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();

  void backspace() { screen.PostEvent(ftxui::Event::Backspace); }

  void refresh_screen() { screen.PostEvent(ftxui::Event::Character('m')); }

  void update_binary(const std::string &_binary_path) {
    static_debugger = std::optional<ElfReader>(_binary_path);
    dynamic_debugger = std::optional<ElfRunner>(_binary_path);
    binary_path = _binary_path;
    function_name = "_start";
    refresh_screen();
  }

  void update_function(const std::string &_function_name) {
    if (dynamic_debugger.has_value()) {
      dynamic_debugger->reset();
    }
    function_name = _function_name;
    refresh_screen();
  }

  void run_function() {
    const bool is_pie = static_debugger->is_position_independent();
    const uint64_t base_address = dynamic_debugger->get_base_address(is_pie);
    const auto function =
        static_debugger->get_function(function_name, base_address);
    const auto function_calls =
        static_debugger->get_function_calls(function_name, base_address);

    dynamic_debugger->run_function(function, function_calls, base_address);
  }

  void run_all_functions() {
    const bool is_pie = static_debugger->is_position_independent();
    const uint64_t base_address = dynamic_debugger->get_base_address(is_pie);
    const auto functions =
        static_debugger->get_implemented_functions(base_address);
    dynamic_debugger->run_functions(functions, base_address);
  }
};
