#pragma once

#include "elf_reader.hpp"
#include "elf_runner.hpp"
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

  void backspace();
  void refresh_screen();

  void update_binary(const std::string &_binary_path);
  void update_function(const std::string &_function_name);

  void run_function();
  void run_all_functions();
};
