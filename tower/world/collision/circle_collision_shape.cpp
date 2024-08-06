#include <glm/geometric.hpp>
#include <tower/world/collision/circle_collision_shape.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::world {
CircleCollisionShape::CircleCollisionShape(const float radius)
    : radius {radius} {}

bool CircleCollisionShape::is_colliding(const CollisionShape* other) const {
    const auto center = get_global_position();

    if (const auto other_rect = dynamic_cast<const RectangleCollisionShape*>(other)) {
        const auto other_center = other_rect->get_global_position();
        const auto other_size = other_rect->get_rotated_size();

        return distance(center, other_center + glm::vec2 {other_size.x, other_size.y}) <= radius ||
            distance(center, other_center + glm::vec2 {-other_size.x, other_size.y}) <= radius ||
            distance(center, other_center + glm::vec2 {other_size.x, -other_size.y}) <= radius ||
            distance(center, other_center + glm::vec2 {-other_size.x, -other_size.y}) <= radius;
    }

    if (const auto other_circle = dynamic_cast<const CircleCollisionShape*>(other)) {
        const auto other_center = other_circle->get_global_position();

        return distance(center, other_center) <= radius + other_circle->radius;
    }

    return false;
}

bool CircleCollisionShape::is_colliding(const glm::vec2& point) const {
    const auto center = get_global_position();
    return distance(center, point) <= radius;
}
}
