#pragma once

#include <ftxui/component/component.hpp>
#include "dependency.hpp"
#include "app_state.hpp"


class MultiTrace : Dependency<AppState>
{
    using Dependency::Dependency;
public:
    ftxui::Component get_logic();

private:
    ftxui::Component _generate_logic();

private:
    ftxui::Component _logic = _generate_logic();
};

