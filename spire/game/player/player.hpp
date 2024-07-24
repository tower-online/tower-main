#pragma once

#include <spire/game/entity.hpp>
#include <spire/game/player/inventory.hpp>
#include <spire/system/event.hpp>
#include <spire/world/world.hpp>

namespace spire::game::player {
using namespace world;

class Player : public Entity {
public:
    void initialize();

    constexpr static float MOVEMENT_SPEED = 10.0f;

    Inventory inventory {};
};
}
