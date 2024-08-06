#pragma once

#include <tower/world/entity/entity.hpp>
#include <tower/world/collision/collision_object.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::game {
using namespace world;

class Piggy : public Entity {
public:
    Piggy();

    static std::shared_ptr<Entity> create();
    void tick() override;
};

inline Piggy::Piggy()
    : Entity {EntityType::PIGGY} {}

inline std::shared_ptr<Entity> Piggy::create() {
    auto piggy = std::make_shared<Piggy>();

    piggy->movement_speed_base = 10.0f;
    piggy->resource.max_health = 50.0f;
    piggy->resource.health = piggy->resource.max_health;

    const auto body_collider = CollisionObject::create(
        std::make_shared<RectangleCollisionShape>(glm::vec2 {12, 12}),
        static_cast<uint32_t>(ColliderLayer::ENTITIES),
        static_cast<uint32_t>(ColliderLayer::PLAYERS)
    );
    piggy->add_child(body_collider);

    return piggy;
}

inline void Piggy::tick() {

}
}
