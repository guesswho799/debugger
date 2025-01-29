#include "ui/tab_controller.hpp"
#include "loader.hpp"
#include "ui/info.hpp"
#include <cmath>
#include <cstdint>

ftxui::Component TabController::get_logic() { return _logic; }

TabController::State_t TabController::_generate_initial_state() {
  auto tab_option = ftxui::MenuOption::HorizontalAnimated();
  tab_option.elements_infix = [] { return ftxui::text(" | "); };
  tab_option.entries_option.transform = [](const ftxui::EntryState &state) {
    ftxui::Element e =
        ftxui::text(state.label) | ftxui::color(ftxui::Color::Blue);
    if (state.active)
      e = e | ftxui::bold | ftxui::underlined;
    return e;
  };

  auto toggle_option = ftxui::MenuOption::HorizontalAnimated();
  toggle_option.elements_infix = [] { return ftxui::text(" | "); };
  toggle_option.entries_option.transform = [](const ftxui::EntryState &state) {
    ftxui::Element e = ftxui::text(state.label);
    if (state.active)
      e = e | ftxui::bold | ftxui::inverted;
    return e;
  };

  return {.tab_values = {"Display", "Execute"},
          .display_values = {"Main", "Open file", "Functions", "Strings"},
          .execute_values = {"Information", "All functions", "Single function"},
          .tab_toggle =
              ftxui::Menu(&_state.tab_values, &get<AppState>()->tab_selected,
                          toggle_option),
          .display_toggle =
              ftxui::Menu(&_state.display_values,
                          &get<AppState>()->display_selected, tab_option),
          .execute_toggle =
              ftxui::Menu(&_state.execute_values,
                          &get<AppState>()->execute_selected, tab_option),
          .display_main = ftxui::Container::Tab(
              {get<Code>()->get_logic(), get<File>()->get_logic(),
               get<Function>()->get_logic(), get<String>()->get_logic()},
              &get<AppState>()->display_selected),
          .execute_main = ftxui::Container::Tab(
              {Info::get_logic(), get<MultiTrace>()->get_logic(),
               get<SingleTrace>()->get_logic()},
              &get<AppState>()->execute_selected),
          .tab_container = ftxui::Container::Tab(
              {ftxui::Renderer([&] {
                 return ftxui::vbox(
                     {ftxui::hbox(
                          {ftxui::text(" "), _state.display_toggle->Render()}) |
                          ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1),
                      _state.display_main->Render()});
               }),
               ftxui::Renderer([&] {
                 return ftxui::vbox(
                     {ftxui::hbox(
                          {ftxui::text(" "), _state.execute_toggle->Render()}) |
                          ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1),
                      _state.execute_main->Render()});
               })},
              &get<AppState>()->tab_selected)};
}

ftxui::Component TabController::_generate_logic() {
  return ftxui::Renderer(
             _state.display_main,
             [&] {
               return ftxui::vbox(
                   {ftxui::hbox({_state.tab_toggle->Render() | ftxui::flex,
                                 ftxui::text("Quit")}),
                    _state.tab_container->Render()});
             }) |
         ftxui::CatchEvent([&](ftxui::Event event) {
           if (get<AppState>()->in_search_mode)
             return false;
           if (event == ftxui::Event::Character('d')) {
             get<AppState>()->tab_selected = 0;
             get<AppState>()->display_selected = 0;
             get<Code>()->reset_selector();
             get<Code>()->update_code();
           } else if (event == ftxui::Event::Character('e')) {
             get<AppState>()->tab_selected = 1;
             get<AppState>()->execute_selected = 0;
           } else if (event == ftxui::Event::Character('m')) {
             get<AppState>()->display_selected = 0;
           } else if (event == ftxui::Event::Character('o')) {
             get<AppState>()->display_selected = 1;
           } else if (event == ftxui::Event::Character('f')) {
             get<AppState>()->display_selected = 2;
           } else if (get<AppState>()->tab_selected == 0 and
                      event == ftxui::Event::Character('s')) {
             get<AppState>()->display_selected = 3;
           } else if (event == ftxui::Event::Character('i')) {
             get<AppState>()->execute_selected = 0;
           } else if (event == ftxui::Event::Character('a')) {
             get<AppState>()->execute_selected = 1;
             get<AppState>()->dynamic_debugger.value().reset();
           } else if (get<AppState>()->tab_selected == 1 and
                      event == ftxui::Event::Character('s')) {
             get<AppState>()->execute_selected = 2;
             get<SingleTrace>()->disable_result();
             get<SingleTrace>()->reset_selector();
             get<Code>()->reset_selector();
             get<AppState>()->dynamic_debugger.value().reset();
           }
           if (get<AppState>()->tab_selected == 1 and
               get<AppState>()->execute_selected == 2 and
               event == ftxui::Event::Character('v')) {
             get<SingleTrace>()->enable_result();
           } else if (event == ftxui::Event::Character('q')) {
             get<AppState>()->screen.ExitLoopClosure()();
             return true;
           }
           if (get<AppState>()->tab_selected == 1 and
               get<AppState>()->execute_selected == 2) {
             if (event == ftxui::Event::ArrowLeft) {
               if (get<SingleTrace>()->get_selector() != 0)
                 get<SingleTrace>()->dec_selector();
               return true;
             } else if (event == ftxui::Event::ArrowRight) {
               get<SingleTrace>()->inc_selector();
               const auto container =
                   get<AppState>()->dynamic_debugger.value().get_runtime_regs();
               const uint64_t code_size =
                   container.end() - container.begin() - 1;
               if (get<SingleTrace>()->get_selector() > code_size)
                 get<SingleTrace>()->dec_selector();
               return true;
             }
           }
           if ((get<AppState>()->tab_selected == 1 and
                get<AppState>()->execute_selected == 2) or
               (get<AppState>()->tab_selected == 0 and
                get<AppState>()->display_selected == 0)) {
             if (event == ftxui::Event::ArrowDown) {
               const auto assembly = get<AppState>()
                                         ->static_debugger.value()
                                         .get_function_code_by_name(
                                             get<AppState>()->function_name);
               const auto assembly_size = static_cast<uint32_t>(assembly.end() - assembly.begin());
               const bool is_single_trace =
                   get<AppState>()->tab_selected == 1 and
                   get<AppState>()->execute_selected == 2;
               const auto [code_size, _] = Loader::max_instruction_height(
                   assembly, get<Code>()->get_selector(), is_single_trace);
               if (code_size + get<Code>()->get_selector() != assembly_size)
                 get<Code>()->inc_selector();
               get<Code>()->update_code();
               return true;
             } else if (event == ftxui::Event::ArrowUp) {
               if (get<Code>()->get_selector() != 0)
                 get<Code>()->dec_selector();
               get<Code>()->update_code();
               return true;
             }
           }
           return false;
         });
}
