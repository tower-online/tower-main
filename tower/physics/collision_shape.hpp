#pragma once

#include <tower/world/world_object.hpp>

namespace tower::physics {
struct CollisionShape : world::WorldObject {
    CollisionShape() = default;

    [[nodiscard]] virtual bool is_colliding(const CollisionShape* other) const = 0;
    [[nodiscard]] virtual bool is_colliding(const glm::vec3& p) const = 0;
};
}
