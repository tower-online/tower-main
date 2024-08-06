#pragma once

#include <tower/item/equipment/equipment.hpp>
#include <tower/world/zone.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::item::equipment {
using namespace world;

class Fist : public Equipment {
public:
    const int32_t damage = 10.0f;
    std::shared_ptr<CollisionShape> attack_shape {std::make_shared<RectangleCollisionShape>(glm::vec2 {8, 16})};
};
}
