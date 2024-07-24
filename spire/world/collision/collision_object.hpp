#pragma once

#include <spire/world/node.hpp>
#include <spire/world/collision/collision_shape.hpp>

namespace spire::world {
enum class ColliderLayer : uint32_t {
    NONE     = 0,
    ENTITIES = 1 << 0,
    TRIGGERS = 1 << 1,
};

class CollisionObject : public Node {
public:
    explicit CollisionObject(uint32_t layer = 0, uint32_t mask = 0);

    void set_shape(const std::shared_ptr<CollisionShape>& shape);

    CollisionShape* shape = nullptr;
    uint32_t layer;
    uint32_t mask;
};

inline CollisionObject::CollisionObject(const uint32_t layer,
    const uint32_t mask)
    : layer {layer}, mask {mask} {}

inline void CollisionObject::set_shape(const std::shared_ptr<CollisionShape>& shape) {
    this->shape = shape.get();
    add_child(shape);
}
}
