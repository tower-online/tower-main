#pragma once

#include <tower/units.hpp>
#include <tower/physics/collision_shape.hpp>

namespace tower::physics {
struct SphereCollisionShape final : CollisionShape {
    explicit SphereCollisionShape(meters radius);

    [[nodiscard]] bool is_colliding(const CollisionShape* other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec3& p) const override;

    const meters radius;
};
}