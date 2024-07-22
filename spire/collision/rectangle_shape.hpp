#pragma once

#include <glm/geometric.hpp>
#include <spire/collision/circle_shape.hpp>
#include <spire/collision/collision_shape.hpp>

namespace spire {
struct RectangleShape : CollisionShape {
    RectangleShape(const glm::vec2& center, const glm::vec2& size);

    [[nodiscard]] bool is_colliding(const CollisionShape& other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    glm::vec2 center;
    // {width // 2, height // 2}
    glm::vec2 size;
};

inline RectangleShape::RectangleShape(const glm::vec2& center, const glm::vec2& size)
    : center {center}, size {size} {}

inline bool RectangleShape::is_colliding(const CollisionShape& other) const {
    if (const auto rother = dynamic_cast<const RectangleShape*>(&other)) {
        return !(
            center.x + size.x < rother->center.x - rother->size.x ||
            center.x - size.x > rother->center.x + rother->size.x ||
            center.y + size.y < rother->center.y - rother->size.y ||
            center.y - size.y > rother->center.y + rother->size.y);
    }
    if (const auto cother = dynamic_cast<const CircleShape*>(&other)) {
        return distance(cother->center, center + glm::vec2 {size.x, size.y}) <= cother->radius ||
            distance(cother->center, center + glm::vec2 {-size.x, size.y}) <= cother->radius ||
            distance(cother->center, center + glm::vec2 {size.x, -size.y}) <= cother->radius ||
            distance(cother->center, center + glm::vec2 {-size.x, -size.y}) <= cother->radius;
    }
    return false;
}

inline bool RectangleShape::is_colliding(const glm::vec2& point) const {
    return point.x >= center.x - size.x && point.x <= center.x + size.x &&
        point.y >= center.y - size.y && point.y <= center.y + size.y;
}
}
