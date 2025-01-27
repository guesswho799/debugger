#pragma once

#include <ftxui/component/component.hpp>

namespace Info {
inline ftxui::Component get_logic() {
  return ftxui::Renderer([] {
    return ftxui::vbox(
               {ftxui::text("Program execution tab"), ftxui::separator(),
                ftxui::text("Press A for a summary of all functions hit"),
                ftxui::text(
                    "Press S for a detailed summary of a selected function")}) |
           ftxui::border | ftxui::center;
  });
}
} // namespace Info
