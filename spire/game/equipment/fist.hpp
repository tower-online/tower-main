#pragma once

#include <spire/game/equipment/equipment.hpp>
#include <spire/world/world.hpp>
#include <spire/world/collision/rectangle_collision_shape.hpp>

namespace spire::game::equipment {
using namespace world;

class Fist : public Equipment {
public:
    Fist();

    std::shared_ptr<CollisionShape> attack_shape {std::make_shared<RectangleCollisionShape>(glm::vec2 {8, 8})};
};

inline Fist::Fist() {
    // offset
    attack_shape->position = {10, 0};
}
}
