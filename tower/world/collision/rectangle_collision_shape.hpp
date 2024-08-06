#pragma once

#include <tower/world/collision/collision_shape.hpp>

namespace tower::world {
struct RectangleCollisionShape : CollisionShape {
    explicit RectangleCollisionShape(const glm::vec2& size);

    // Assumes that global rotation is always aligned with 4-way angle
    [[nodiscard]] bool is_colliding(const CollisionShape* other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    glm::vec2 get_rotated_size() const;

    // {width // 2, height // 2}
    glm::vec2 size;
};
}
