#pragma once

#include <tower/game/equipment/equipment.hpp>
#include <tower/world/world.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::game::equipment {
using namespace world;

class Fist : public Equipment {
public:
    Fist();

    const int32_t damage = 10.0f;
    std::shared_ptr<CollisionShape> attack_shape {std::make_shared<RectangleCollisionShape>(glm::vec2 {8, 8})};
};

inline Fist::Fist() {
    // offset
    attack_shape->position = {10, 0};
}
}
