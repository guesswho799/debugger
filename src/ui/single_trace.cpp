#include "ui/single_trace.hpp"
#include "loader.hpp"
#include <ftxui/dom/elements.hpp>

ftxui::Component SingleTrace::get_logic() { return _logic; }

void SingleTrace::enable_result() { _state.view_result = true; }

void SingleTrace::disable_result() { _state.view_result = false; }

uint64_t SingleTrace::get_selector() { return _state.selector; }
void SingleTrace::inc_selector() { _state.selector++; }
void SingleTrace::dec_selector() { _state.selector--; }
void SingleTrace::reset_selector() { _state.selector = 0; }

SingleTrace::State SingleTrace::_generate_initial_state() {
  return {.view_result = false, .selector = 0};
}

ftxui::Component SingleTrace::_generate_logic() {
  return ftxui::Renderer([&] {
    const bool is_tracee_dead = get<AppState>()->dynamic_debugger->is_dead();
    if (is_tracee_dead or _state.view_result) {
      const auto result_screen = _display_result();
      return result_screen;
    }

    get<AppState>()->refresh_screen();
    get<AppState>()->run_function();

    return _display_status();
  });
}

ftxui::Element SingleTrace::_display_result() {
  const auto assembly =
      get<AppState>()->static_debugger->get_function_code_by_name(
          get<AppState>()->function_name);
  const auto registers =
      get<AppState>()->dynamic_debugger->get_runtime_regs()[_state.selector];
  const auto code_block =
      Loader::load_instructions(get<AppState>()->function_name, assembly,
                                get<Code>()->get_selector(), registers.rip);
  const auto trace_player = Loader::load_trace_player(
      get<AppState>()->dynamic_debugger->get_runtime_regs(), _state.selector);
  const auto register_window = Loader::load_register_window(registers);
  const auto stack_window = Loader::load_stack_window(
      get<AppState>()->dynamic_debugger->get_runtime_stacks()[_state.selector]);
  return ftxui::vbox(
             {trace_player,
              ftxui::hbox({ftxui::vbox({code_block, ftxui::filler()}),
                           ftxui::vbox({register_window, stack_window})}) |
                  ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN,
                              ftxui::Terminal::Size().dimy - 9)}) |
         ftxui::center;
}

ftxui::Element SingleTrace::_display_status() {
  std::string hit_status;
  if (get<AppState>()->dynamic_debugger.value().get_runtime_regs().empty())
    hit_status = "not hit yet";
  else
    hit_status = "hit. Press V to view results";

  std::string status = std::format("Selected function ({}), {}",
                                   get<AppState>()->function_name, hit_status);

  return ftxui::vbox({ftxui::text("Running process..."), ftxui::text(status)}) |
         ftxui::border | ftxui::center;
}
