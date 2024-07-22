#pragma once

#include <spire/collision/collision_shape.hpp>
#include <spire/collision/rectangle_shape.hpp>

namespace spire {
struct CircleShape : CollisionShape {
    CircleShape(const glm::vec2& center, float radius);

    [[nodiscard]] bool is_colliding(const CollisionShape& other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    glm::vec2 center;
    float radius;
};

inline CircleShape::CircleShape(const glm::vec2& center, float radius)
    : center {center}, radius {radius} {}

inline bool CircleShape::is_colliding(const CollisionShape& other) const {
    return false;
}

inline bool CircleShape::is_colliding(const glm::vec2& point) const {
    return false;
}
}
