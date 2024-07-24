#pragma once

#include <spire/game/equipment/equipment.hpp>
#include <spire/world/world.hpp>
#include <spire/world/collision/rectangle_collider.hpp>

namespace spire::game::equipment {
using namespace world;

class Fist : public Equipment {
public:
    std::shared_ptr<Collider> attack_collider {
        std::make_shared<RectangleCollider>(
            glm::vec2 {9, 2}, glm::vec2 {8, 17},
            static_cast<uint32_t>(ColliderLayer::TRIGGERS), static_cast<uint32_t>(ColliderLayer::ENTITIES))
    };
};
}
