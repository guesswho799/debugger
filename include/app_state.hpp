#pragma once

#include "elf_reader.hpp"
#include "elf_runner.hpp"
#include <ftxui/component/screen_interactive.hpp>

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
    refresh_screen();
  }

  void update_function(const std::string &_function_name) {
    if (dynamic_debugger.has_value()) {
      dynamic_debugger->reset();
    }
    function_name = _function_name;
    refresh_screen();
  }
};
