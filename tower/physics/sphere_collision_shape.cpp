#include <glm/geometric.hpp>
#include <tower/physics/cube_collision_shape.hpp>
#include <tower/physics/sphere_collision_shape.hpp>

namespace tower::physics {
SphereCollisionShape::SphereCollisionShape(const meters radius)
    : radius {radius} {}

bool SphereCollisionShape::is_colliding(const CollisionShape* other) const {
    const auto center {get_global_position()};

    if (const auto other_cube {dynamic_cast<const CubeCollisionShape*>(other)}) {
        const auto other_center {other_cube->get_global_position()};
        const auto& other_size {other_cube->size};
        const glm::vec3 other_min {
            other_center.x - other_size.x, other_center.y - other_size.y, other_center.z - other_size.z};
        const glm::vec3 other_max {
            other_center.x + other_size.x, other_center.y + other_size.y, other_center.z + other_size.z};

        const glm::vec3 closest {
            glm::max(other_min.x, glm::min(center.x, other_max.x)),
            glm::max(other_min.y, glm::min(center.y, other_max.y)),
            glm::max(other_min.z, glm::min(center.z, other_max.z)),
        };

        return distance(closest, center) <= radius;
    }

    if (const auto other_sphere {dynamic_cast<const SphereCollisionShape*>(other)}) {
        const auto other_center {other_sphere->get_global_position()};

        return distance(center, other_center) <= radius + other_sphere->radius;
    }

    return false;
}

bool SphereCollisionShape::is_colliding(const glm::vec3& p) const {
    const auto center {get_global_position()};
    return distance(center, p) <= radius;
}
}
