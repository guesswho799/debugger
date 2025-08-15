#include "ui/file.hpp"
#include <filesystem>
#include <ranges>

ftxui::Component File::get_logic() { return _logic; }

File::State File::_generate_initial_state() {
  return {
      .file_path = "",
      .file_input = ftxui::Input(&_state.file_path, ""),
      .files = {},
      .files_as_strings = {},
      .open_file_selector = 0,
  };
};

ftxui::Component File::_generate_logic() {
  return ftxui::Renderer(
             _state.file_input,
             [&] {
               _store_files_from_input();

               auto menu = ftxui::Container::Vertical(
                   _state.files, &_state.open_file_selector);
               if (get<AppState>()->in_search_mode) {
                 return ftxui::vbox({ftxui::window(ftxui::text("Search file"),
                                                   _state.file_input->Render()),
                                     ftxui::separator(),
                                     ftxui::window(ftxui::text("Files"),
                                                   menu->Render())}) |
                        ftxui::border;
               } else {
                 return ftxui::vbox(
                            {ftxui::text(" Press / to start searching..."),
                             ftxui::separator(),
                             ftxui::window(ftxui::text("Files"),
                                           menu->Render())}) |
                        ftxui::border;
               }
             }) |
         ftxui::CatchEvent([&](ftxui::Event event) {
           if (event == ftxui::Event::Escape) {
             get<AppState>()->in_search_mode = false;
           }
           if (event == ftxui::Event::Character('/') and
               get<AppState>()->in_search_mode == false) {
             get<AppState>()->in_search_mode = true;
             _state.file_path = "";
             get<AppState>()->backspace();
           }
           if (get<AppState>()->in_search_mode) {
             if (event == ftxui::Event::ArrowUp or
                 event == ftxui::Event::TabReverse)
               _state.open_file_selector--;
             if (event == ftxui::Event::ArrowDown or event == ftxui::Event::Tab)
               _state.open_file_selector++;
             if (_state.open_file_selector < 0)
               _state.open_file_selector = 0;
             const int files_size = static_cast<int>(_state.files.end() -
                                                     _state.files.begin() - 1);
             if (_state.open_file_selector > files_size)
               _state.open_file_selector = files_size;
             if (event == ftxui::Event::Return) {
               get<AppState>()->update_binary(
                   _state.files_as_strings[_state.open_file_selector]);
               get<Code>()->update_code();
               get<AppState>()->in_search_mode = false;
               _state.file_path = "";
             }
           }
           return false;
         });
}

void File::_store_files_from_input() {
  _state.files.clear();
  _state.files_as_strings.clear();

  std::string search_path = _get_diretory_name(_state.file_path);
  if (std::filesystem::is_directory(search_path)) {
    for (const auto &file :
         std::filesystem::recursive_directory_iterator(search_path) |
             std::views::take(20)) {
      _append_file(file.path().string());
    }
  } else {
    _append_file(search_path);
  }
}

std::string File::_get_diretory_name(const std::string &file_name) {
  if (!std::filesystem::exists(file_name)) {
    const size_t last_directory_index = file_name.find_last_of('/');
    if (last_directory_index == std::string::npos) {
      return ".";
    } else {
      return std::string(_state.file_path.c_str(), last_directory_index);
    }
  }
  return file_name;
}

void File::_append_file(const std::string &file_name) {
  _state.files.emplace_back(ftxui::MenuEntry(file_name));
  _state.files_as_strings.emplace_back(file_name);
}
