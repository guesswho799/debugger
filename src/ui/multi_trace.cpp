#include "ui/multi_trace.hpp"
#include "loader.hpp"

ftxui::Component MultiTrace::get_logic() { return _logic; }

ftxui::Component MultiTrace::_generate_logic() {
  return ftxui::Renderer([&] {
    std::string title;
    if (!get<AppState>()->dynamic_debugger.value().is_dead()) {
      title = "Running process...";
      get<AppState>()->run_all_functions();
      get<AppState>()->refresh_screen();
    } else {
      title = "Program finished";
    }
    const auto runtime_value =
        get<AppState>()->dynamic_debugger.value().get_runtime_arguments();
    return ftxui::vbox({ftxui::text(title), ftxui::separator(),
                        Loader::load_functions_arguments(runtime_value)}) |
           ftxui::border | ftxui::center;
  });
}
