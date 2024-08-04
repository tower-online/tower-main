#pragma once

#include <tower/system/event.hpp>
#include <tower/world/node.hpp>
#include <tower/world/collision/collision_shape.hpp>

namespace tower::world {
enum class ColliderLayer : uint32_t {
    NONE     = 0,
    ENTITIES = 1 << 0,
    TRIGGERS = 1 << 1,
};

class CollisionObject : public Node {
public:
    CollisionObject(const CollisionShape* shape, uint32_t layer, uint32_t mask);

    static std::shared_ptr<CollisionObject> create(std::shared_ptr<CollisionShape>&& shape,
        uint32_t layer = 0, uint32_t mask = 0);

    const CollisionShape* shape;
    uint32_t layer;
    uint32_t mask;

    Event<std::shared_ptr<Node>> collision_entered;
    Event<std::shared_ptr<Node>> collision_staying;
    Event<std::shared_ptr<Node>> collision_exitied;
};


inline std::shared_ptr<CollisionObject> CollisionObject::create(std::shared_ptr<CollisionShape>&& shape,
    const uint32_t layer, const uint32_t mask) {
    auto o = std::make_shared<CollisionObject>(shape.get(), layer, mask);
    o->add_child(std::move(shape));

    return o;
}

inline CollisionObject::CollisionObject(const CollisionShape* shape,
    const uint32_t layer, const uint32_t mask)
    : shape {shape}, layer {layer}, mask {mask} {}
}
