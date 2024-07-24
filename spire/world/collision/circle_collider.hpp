#pragma once

#include <spire/world/collision/collider.hpp>
#include <spire/world/collision/rectangle_collider.hpp>

namespace spire::world {
struct CircleCollider : Collider {
    CircleCollider(std::shared_ptr<Node>& parent, const glm::vec2& center, float radius);

    [[nodiscard]] bool is_colliding(std::shared_ptr<Collider>& other) const override;
    [[nodiscard]] bool is_colliding(const glm::vec2& point) const override;

    glm::vec2 center;
    float radius;
};

inline CircleCollider::CircleCollider(std::shared_ptr<Node>& parent, const glm::vec2& center, float radius)
    : center {center}, radius {radius} {}

inline bool CircleCollider::is_colliding(std::shared_ptr<Collider>& other) const {
    return false;
}

inline bool CircleCollider::is_colliding(const glm::vec2& point) const {
    return false;
}
}
