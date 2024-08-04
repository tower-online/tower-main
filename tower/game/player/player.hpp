#pragma once

#include <tower/game/player/inventory.hpp>
#include <tower/world/entity.hpp>

namespace tower::game::player {
using namespace world;

class Player : public Entity {
public:
    Player();

    static std::shared_ptr<Player> create();

    std::shared_ptr<Node> pivot {std::make_shared<Node>()};
    Inventory inventory {};
};
}
