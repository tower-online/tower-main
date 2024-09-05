#pragma once

#include <tower/player/inventory.hpp>
#include <tower/entity/entity.hpp>

namespace tower::player {
using namespace tower::entity;
using namespace tower::world;

class Player : public Entity {
public:
    Player();

    static std::shared_ptr<Player> create();

    void tick(Subworld& subworld) override {}

    std::shared_ptr<Node> pivot {std::make_shared<Node>()};
    Inventory inventory {};
};
}
