#include <glm/geometric.hpp>
#include <tower/system/math.hpp>
#include <tower/world/collision/circle_collision_shape.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::world {
RectangleCollisionShape::RectangleCollisionShape(const glm::vec2& size)
    : size {size} {}

bool RectangleCollisionShape::is_colliding(const CollisionShape* other) const {
    const auto center = get_global_position();
    const auto rotated_size = get_rotated_size();

    if (const auto other_rect = dynamic_cast<const RectangleCollisionShape*>(other)) {
        const auto other_center = other_rect->get_global_position();
        const auto other_size = other_rect->get_rotated_size();
        return !(
            center.x + rotated_size.x < other_center.x - other_size.x ||
            center.x - rotated_size.x > other_center.x + other_size.x ||
            center.y + rotated_size.y < other_center.y - other_size.y ||
            center.y - rotated_size.y > other_center.y + other_size.y);
    }

    if (const auto other_circle = dynamic_cast<const CircleCollisionShape*>(other)) {
        const auto other_center = other_circle->get_global_position();
        return distance(other_center, center + glm::vec2 {rotated_size.x, rotated_size.y}) <= other_circle->radius ||
            distance(other_center, center + glm::vec2 {-rotated_size.x, rotated_size.y}) <= other_circle->radius ||
            distance(other_center, center + glm::vec2 {rotated_size.x, -rotated_size.y}) <= other_circle->radius ||
            distance(other_center, center + glm::vec2 {-rotated_size.x, -rotated_size.y}) <= other_circle->radius;
    }

    return false;
}

bool RectangleCollisionShape::is_colliding(const glm::vec2& point) const {
    const auto center = get_global_position();
    return point.x >= center.x - size.x && point.x <= center.x + size.x &&
        point.y >= center.y - size.y && point.y <= center.y + size.y;
}

glm::vec2 RectangleCollisionShape::get_rotated_size() const {
    if (const auto global_rotation = get_global_rotation();
        f_is_equal(global_rotation, 0.0f) || f_is_equal(global_rotation, PI)) {
        return size;
    }
    return {size.y, size.x};
}
}