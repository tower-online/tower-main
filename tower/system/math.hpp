#pragma once

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <cmath>
#include <numbers>

namespace tower {
constexpr float PI = std::numbers::pi_v<float>;
constexpr float PI_HALF = PI / 2.0f;

inline float direction_to_4way_angle(const glm::vec2& direction) {
    if (direction.x > 0.0) return 0.0;
    if (direction.x < 0.0) return PI;
    if (direction.y > 0.0) return PI_HALF;
    if (direction.y < 0.0) return PI + PI_HALF;
    return 0.0;
}

inline bool f_is_equal(const float a, const float b, const float epsilon = 1e-6f) {
    return std::fabs(a - b) <= epsilon;
}
}
