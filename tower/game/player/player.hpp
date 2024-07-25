#pragma once

#include <tower/game/entity.hpp>
#include <tower/game/player/inventory.hpp>
#include <tower/system/event.hpp>

namespace tower::game::player {
using namespace world;

class Player : public Entity {
public:
    constexpr static float MOVEMENT_SPEED = 10.0f;

    Player() = default;

    static std::shared_ptr<Player> create();

    Inventory inventory {};
};
}
