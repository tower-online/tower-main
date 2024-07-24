#pragma once

#include <spire/world/node.hpp>

#include <atomic>

namespace spire::world {
enum class ColliderLayer : uint32_t {
    NONE     = 0,
    ENTITIES = 1 << 0,
    TRIGGERS = 1 << 1,
};


struct Collider : Node {
    Collider();
    Collider(uint32_t layer, uint32_t mask);

    [[nodiscard]] virtual bool is_colliding(std::shared_ptr<Collider>& other) const = 0;
    [[nodiscard]] virtual bool is_colliding(const glm::vec2& point) const = 0;

    static uint32_t generate_id();

    const uint32_t id;
    uint32_t layer {0};
    uint32_t mask {0};
};

inline Collider::Collider()
    : id {generate_id()} {}

inline Collider::Collider(const uint32_t layer, const uint32_t mask)
    : id {generate_id()}, layer {layer}, mask {mask} {}

inline uint32_t Collider::generate_id() {
    static std::atomic<uint32_t> id_generator {0};
    return ++id_generator;
}
}
