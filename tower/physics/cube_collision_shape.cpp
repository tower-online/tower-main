#include <glm/geometric.hpp>
#include <tower/physics/cube_collision_shape.hpp>
#include <tower/physics/sphere_collision_shape.hpp>

namespace tower::physics {
CubeCollisionShape::CubeCollisionShape(const glm::vec3& size)
    : size {size} {}

bool CubeCollisionShape::is_colliding(const CollisionShape* other) const {
    const auto center {get_global_position()};
    const glm::vec3 min {center.x - size.x, center.y - size.y, center.z - size.z};
    const glm::vec3 max {center.x + size.x, center.y + size.y, center.z + size.z};

    if (const auto other_cube {dynamic_cast<const CubeCollisionShape*>(other)}) {
        const auto other_center {other_cube->get_global_position()};
        const auto& other_size {other_cube->size};
        const glm::vec3 other_min {
            other_center.x - other_size.x, other_center.y - other_size.y, other_center.z - other_size.z};
        const glm::vec3 other_max {
            other_center.x + other_size.x, other_center.y + other_size.y, other_center.z + other_size.z};

        return
            min.x <= other_max.x && max.x >= other_min.x &&
            min.y <= other_max.y && max.y >= other_min.y &&
            min.z <= other_max.z && max.z >= other_min.z;
    }

    if (const auto other_sphere {dynamic_cast<const SphereCollisionShape*>(other)}) {
        const auto other_center {other_sphere->get_global_position()};
        const glm::vec3 closest {
            glm::max(min.x, glm::min(other_center.x, max.x)),
            glm::max(min.y, glm::min(other_center.y, max.y)),
            glm::max(min.z, glm::min(other_center.z, max.z)),
        };

        return distance(closest, other_center) <= other_sphere->radius;
    }

    return false;
}

bool CubeCollisionShape::is_colliding(const glm::vec3& p) const {
    const auto center {get_global_position()};
    const glm::vec3 min {center.x - size.x, center.y - size.y, center.z - size.z};
    const glm::vec3 max {center.x + size.x, center.y + size.y, center.z + size.z};

    return
        p.x >= min.x && p.x <= max.x &&
        p.y >= min.y && p.y <= max.y &&
        p.z >= min.z && p.z <= max.z;
}
}
