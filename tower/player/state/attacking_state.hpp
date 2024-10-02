#pragma once

#include <tower/system/state.hpp>

namespace tower::player {
class AttackingState final : public State {
    static constexpr std::string_view name {"Attacking"};

public:
    void enter() override {}
    void exit() override {}
    std::string_view get_name() const override { return name; }
};
}
