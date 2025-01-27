#include "ui/string.hpp"
#include "loader.hpp"

ftxui::Component String::get_logic() { return _logic; }

String::State_t String::_generate_initial_state() {
  float scroll_x = 0.f;
  float scroll_y = 0.f;
  ftxui::SliderOption<float> option_x;
  option_x.value = &scroll_x;
  option_x.min = 0.f;
  option_x.max = 1.f;
  option_x.increment = 0.5f;
  option_x.direction = ftxui::Direction::Right;
  option_x.color_active = ftxui::Color::Black;
  option_x.color_inactive = ftxui::Color::Black;
  auto scrollbar_x = ftxui::Slider(option_x);
  ftxui::SliderOption<float> option_y;
  option_y.value = &scroll_y;
  option_y.min = 0.f;
  option_y.max = 1.f;
  option_y.increment = 0.01f;
  option_y.direction = ftxui::Direction::Down;
  option_y.color_active = ftxui::Color::Black;
  option_y.color_inactive = ftxui::Color::Black;
  auto scrollbar_y = ftxui::Slider(option_y);
  return {.scroll_x = scroll_x,
          .scroll_y = scroll_y,
          .option_x = option_x,
          .option_y = option_y,
          .scrollbar_x = scrollbar_x,
          .scrollbar_y = scrollbar_y};
}

ftxui::Component String::_generate_logic() {
  // TODO: fix slider acting wierd,
  // probably need to add main component to Renderer
  auto string_tab = ftxui::Renderer([&] {
    return Loader::load_strings(get<AppState>()->static_debugger.value()) |
           ftxui::focusPositionRelative(_state.scroll_x, _state.scroll_y) |
           ftxui::vscroll_indicator | ftxui::frame | ftxui::flex;
  });
  return ftxui::Container::Horizontal({string_tab, _state.scrollbar_y}) |
         ftxui::flex;
}
