#pragma once

#include <spire/world/node.hpp>

namespace spire::world {
struct CollisionShape : Node {
    CollisionShape() = default;

    [[nodiscard]] virtual bool is_colliding(const CollisionShape* other) const = 0;
    [[nodiscard]] virtual bool is_colliding(const glm::vec2& point) const = 0;
};
}
