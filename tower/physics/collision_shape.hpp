#pragma once

#include <tower/world/node.hpp>

namespace tower::physics {
struct CollisionShape : world::Node {
    CollisionShape() = default;

    [[nodiscard]] virtual bool is_colliding(const CollisionShape* other) const = 0;
    [[nodiscard]] virtual bool is_colliding(const glm::vec3& p) const = 0;
};
}
