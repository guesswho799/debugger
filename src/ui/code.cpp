#include "ui/code.hpp"
#include "loader.hpp"

ftxui::Component Code::get_logic() { return _logic; }

Code::State_t Code::_generate_initial_state() {
  return {.selector = 0, .disassembled_code = ftxui::text("no file loaded..")};
}

ftxui::Component Code::_generate_logic() {
  return ftxui::Renderer([&] { return _state.disassembled_code; });
}

void Code::update_code() {
  _state.disassembled_code = Loader::load_instructions(
      get<AppState>()->function_name,
      get<AppState>()->static_debugger.value().get_function_code_by_name(
          get<AppState>()->function_name),
      _state.selector);
}

uint64_t Code::get_selector() { return _state.selector; }
void Code::inc_selector() { _state.selector++; }
void Code::dec_selector() { _state.selector--; }
void Code::reset_selector() { _state.selector = 0; }
