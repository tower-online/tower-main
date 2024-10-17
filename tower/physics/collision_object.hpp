#pragma once

#include <boost/signals2.hpp>
#include <tower/physics/collision_shape.hpp>
#include <tower/world/world_object.hpp>

namespace tower::physics {

enum class ColliderLayer : uint32_t {
    NONE     = 0,
    ENTITIES = 1 << 0,
    AREAS = 1 << 1,
    PLAYERS = 1 << 2,
    MOBS = 1 << 3,
};

class CollisionObject : public world::WorldObject {
public:
    CollisionObject(const CollisionShape* shape, uint32_t layer, uint32_t mask);

    static std::shared_ptr<CollisionObject> create(std::shared_ptr<CollisionShape>&& shape,
        uint32_t layer = 0, uint32_t mask = 0);

    bool is_colliding(const std::shared_ptr<CollisionObject>& other) const;

    const CollisionShape* shape;
    uint32_t layer;
    uint32_t mask;

    boost::signals2::signal<void(std::shared_ptr<WorldObject>)> body_entered {};
    boost::signals2::signal<void(std::shared_ptr<WorldObject>)> body_staying {};
    boost::signals2::signal<void(std::shared_ptr<WorldObject>)> body_exited {};
};


inline std::shared_ptr<CollisionObject> CollisionObject::create(std::shared_ptr<CollisionShape>&& shape,
    const uint32_t layer, const uint32_t mask) {
    auto o = std::make_shared<CollisionObject>(shape.get(), layer, mask);
    o->add_child(std::move(shape));

    return o;
}

inline bool CollisionObject::is_colliding(const std::shared_ptr<CollisionObject>& other) const {
    if (this == other.get()) return false;
    if (!(mask & other->layer)) return false;
    return shape->is_colliding(other->shape);
}

inline CollisionObject::CollisionObject(const CollisionShape* shape,
    const uint32_t layer, const uint32_t mask)
    : shape {shape}, layer {layer}, mask {mask} {}

constexpr uint32_t operator|(ColliderLayer a, ColliderLayer b) {
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

constexpr uint32_t operator&(ColliderLayer a, ColliderLayer b) {
    return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}
}
