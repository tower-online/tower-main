#pragma once

#include <numbers>

namespace tower {
using meters = float;
using imeters = int;

using radian = float;
using degree = float;

static constexpr degree rad2deg(const radian x) {
    static constexpr float constant {180.0f / std::numbers::pi};
    return x * constant;
}

static constexpr radian deg2rad(const degree x) {
    static constexpr float constant {std::numbers::pi / 180.0f};
    return x * constant;
}
}