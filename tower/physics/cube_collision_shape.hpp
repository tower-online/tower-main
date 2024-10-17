#pragma once

#include <tower/physics/collision_shape.hpp>

namespace tower::physics {
/**
 * AABB
 */
struct CubeCollisionShape final : CollisionShape {
    explicit CubeCollisionShape(const glm::vec3& size);

    [[nodiscard]] bool is_colliding(const CollisionShape* other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec3& p) const override;

    /**
     * Half lengths of each x,y,z sides
     */
    const glm::vec3 size;
};
}