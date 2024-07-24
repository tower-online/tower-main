#pragma once

#include <glm/geometric.hpp>
#include <spire/world/collision/circle_collision_shape.hpp>
#include <spire/world/collision/collision_shape.hpp>

namespace spire::world {
struct RectangleCollisionShape : CollisionShape {
    explicit RectangleCollisionShape(const glm::vec2& size);

    [[nodiscard]] bool is_colliding(const CollisionShape* other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    // {width // 2, height // 2}
    glm::vec2 size;
};

inline RectangleCollisionShape::RectangleCollisionShape(const glm::vec2& size)
    : size {size} {}

inline bool RectangleCollisionShape::is_colliding(const CollisionShape* other) const {
    const auto center = get_global_position();

    if (const auto rect = dynamic_cast<const RectangleCollisionShape*>(other)) {
        const auto rcenter = rect->get_global_position();
        return !(
            center.x + size.x < rcenter.x - rect->size.x ||
            center.x - size.x > rcenter.x + rect->size.x ||
            center.y + size.y < rcenter.y - rect->size.y ||
            center.y - size.y > rcenter.y + rect->size.y);
    }
    if (const auto circle = dynamic_cast<const CircleCollisionShape*>(other)) {
        const auto ccenter = circle->get_global_position();
        return distance(ccenter, center + glm::vec2 {size.x, size.y}) <= circle->radius ||
            distance(ccenter, center + glm::vec2 {-size.x, size.y}) <= circle->radius ||
            distance(ccenter, center + glm::vec2 {size.x, -size.y}) <= circle->radius ||
            distance(ccenter, center + glm::vec2 {-size.x, -size.y}) <= circle->radius;
    }
    return false;
}

inline bool RectangleCollisionShape::is_colliding(const glm::vec2& point) const {
    const auto center = get_global_position();
    return point.x >= center.x - size.x && point.x <= center.x + size.x &&
        point.y >= center.y - size.y && point.y <= center.y + size.y;
}
}
