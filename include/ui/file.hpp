#pragma once

#include "code.hpp"

class File : Dependency<AppState, Code> {
  using Dependency::Dependency;

private:
  struct State {
    std::string file_path;
    ftxui::Component file_input;
    std::vector<ftxui::Component> files;
    std::vector<std::string> files_as_strings;
    int open_file_selector;
  };

public:
  ftxui::Component get_logic();

private:
  State _generate_initial_state();
  ftxui::Component _generate_logic();

  void _store_files_from_input();
  std::string _get_diretory_name(const std::string &file_name);
  void _append_file(const std::string &file_name);

private:
  State _state = _generate_initial_state();
  ftxui::Component _logic = _generate_logic();
};
