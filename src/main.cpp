#include <cmath>
#include <iostream>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/util/ref.hpp>

#include "app_state.hpp"
#include "status.hpp"
#include "ui/code.hpp"
#include "ui/file.hpp"
#include "ui/function.hpp"
#include "ui/multi_trace.hpp"
#include "ui/single_trace.hpp"
#include "ui/string.hpp"
#include "ui/tab_controller.hpp"

std::shared_ptr<AppState> parse_args(int argc, char *argv[]) {
  auto result = std::make_shared<AppState>();
  if (argc < 2) {
    result->display_selected = 1;
    result->in_search_mode = true;
  }
  if (argc >= 2) {
    result->update_binary(argv[1]);
  }
  if (argc >= 3) {
    result->function_name = argv[2];
  }
  return result;
}

int main(int argc, char *argv[]) {
  try {
    auto app_state = parse_args(argc, argv);
    auto code_tab = std::make_shared<Code>(app_state);
    auto file_tab = std::make_shared<File>(app_state, code_tab);
    auto function_tab = std::make_shared<Function>(app_state, code_tab);
    auto string_tab = std::make_shared<String>(app_state);
    auto multi_trace_tab = std::make_shared<MultiTrace>(app_state);
    auto single_trace_tab = std::make_shared<SingleTrace>(app_state, code_tab);
    TabController tab_controller{app_state,       code_tab,   file_tab,
                                 function_tab,    string_tab, multi_trace_tab,
                                 single_trace_tab};

    app_state->screen.SetCursor(
        ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));
    if (!app_state->binary_path.empty())
      code_tab->update_code();

    app_state->screen.Loop(tab_controller.get_logic());
  } catch (const CriticalException &exception) {
    std::cout << "critical exception caught: " << exception.get() << std::endl;
    return 1;
  }

  return 0;
}
