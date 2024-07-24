#pragma once

#include <glm/geometric.hpp>
#include <spire/world/collision/circle_collider.hpp>
#include <spire/world/collision/collider.hpp>

namespace spire::world {
struct RectangleCollider : Collider {
    RectangleCollider(const glm::vec2& center, const glm::vec2& size);
    RectangleCollider(const glm::vec2& center, const glm::vec2& size, uint32_t layer, uint32_t mask);

    [[nodiscard]] bool is_colliding(std::shared_ptr<Collider>& other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    glm::vec2 center;
    // {width // 2, height // 2}
    glm::vec2 size;
};

inline RectangleCollider::RectangleCollider(const glm::vec2& center, const glm::vec2& size)
    : center {center}, size {size} {}

inline RectangleCollider::RectangleCollider(const glm::vec2& center, const glm::vec2& size, const uint32_t layer,
    const uint32_t mask)
    : Collider {layer, mask}, center {center}, size {size} {}

inline bool RectangleCollider::is_colliding(std::shared_ptr<Collider>& other) const {
    const auto global_center = get_global_position() + center;

    if (const auto rect = std::dynamic_pointer_cast<RectangleCollider>(other)) {
        const auto rect_global_center = rect->get_global_position() + rect->center;
        return !(
            global_center.x + size.x < rect_global_center.x - rect->size.x ||
            global_center.x - size.x > rect_global_center.x + rect->size.x ||
            global_center.y + size.y < rect_global_center.y - rect->size.y ||
            global_center.y - size.y > rect_global_center.y + rect->size.y);
    }
    if (const auto circle = std::dynamic_pointer_cast<CircleCollider>(other)) {
        const auto circle_global_center = circle->get_global_position() + circle->center;
        return distance(circle_global_center, global_center + glm::vec2 {size.x, size.y}) <= circle->radius ||
            distance(circle_global_center, global_center + glm::vec2 {-size.x, size.y}) <= circle->radius ||
            distance(circle_global_center, global_center + glm::vec2 {size.x, -size.y}) <= circle->radius ||
            distance(circle_global_center, global_center + glm::vec2 {-size.x, -size.y}) <= circle->radius;
    }
    return false;
}

inline bool RectangleCollider::is_colliding(const glm::vec2& point) const {
    return point.x >= center.x - size.x && point.x <= center.x + size.x &&
        point.y >= center.y - size.y && point.y <= center.y + size.y;
}
}
