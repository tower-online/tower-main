#pragma once

#include <tower/system/state.hpp>

namespace tower::player {
class JumpingState final : public State {
    static constexpr std::string_view name {"Jumping"};

public:
    void enter() override {}
    void exit() override {}
    std::string_view get_name() const override { return name; }
};
}
