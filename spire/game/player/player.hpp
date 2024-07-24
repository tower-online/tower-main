#pragma once

#include <spire/game/entity.hpp>
#include <spire/game/player/inventory.hpp>
#include <spire/system/event.hpp>

namespace spire::game::player {
using namespace world;

class Player : public Entity {
public:
    constexpr static float MOVEMENT_SPEED = 10.0f;

    Player() = default;

    static std::shared_ptr<Player> create();

    Inventory inventory {};
};
}
