#include "ui/single_trace.hpp"
#include "loader.hpp"

ftxui::Component SingleTrace::get_logic() { return _logic; }

void SingleTrace::enable_result() { _state.view_result = true; }

void SingleTrace::disable_result() { _state.view_result = false; }

uint64_t SingleTrace::get_selector() { return _state.selector; }
void SingleTrace::inc_selector() { _state.selector++; }
void SingleTrace::dec_selector() { _state.selector--; }
void SingleTrace::reset_selector() { _state.selector = 0; }

SingleTrace::State_t SingleTrace::_generate_initial_state() {
  return {.view_result = false, .selector = 0};
}

ftxui::Component SingleTrace::_generate_logic() {
  return ftxui::Renderer([&] {
    if (get<AppState>()->dynamic_debugger->is_dead() or
        _state.view_result == true) {
      const auto assembly =
          get<AppState>()->static_debugger->get_function_code_by_name(
              get<AppState>()->function_name);
      const auto registers =
          get<AppState>()
              ->dynamic_debugger->get_runtime_regs()[_state.selector];
      const auto code_block =
          Loader::load_instructions(get<AppState>()->function_name, assembly,
                                    get<Code>()->get_selector(), registers.rip);
      const auto trace_player = Loader::load_trace_player(
          get<AppState>()->dynamic_debugger->get_runtime_regs(),
          _state.selector);
      const auto register_window = Loader::load_register_window(registers);
      const auto stack_window = Loader::load_stack_window(
          get<AppState>()
              ->dynamic_debugger->get_runtime_stacks()[_state.selector]);
      return ftxui::vbox(
                 {trace_player,
                  ftxui::hbox({code_block,
                               ftxui::vbox({register_window, stack_window})}) |
                      ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN,
                                  ftxui::Terminal::Size().dimy - 9)}) |
             ftxui::center;
    }

    get<AppState>()->refresh_screen();
    get<AppState>()->run_function();

    std::ostringstream function_status;
    function_status << "Selected function (" << get<AppState>()->function_name
                    << ") ";
    if (get<AppState>()->dynamic_debugger.value().get_runtime_regs().empty())
      function_status << "not hit yet";
    else
      function_status << "hit. Press V to view results";
    return ftxui::vbox({ftxui::text("Running process..."),
                        ftxui::text(function_status.str())}) |
           ftxui::border | ftxui::center;
  });
}
