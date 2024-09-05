#pragma once

#include <boost/asio.hpp>
#include <tower/entity/entity.hpp>
#include <tower/player/inventory.hpp>
#include <tower/player/stat.hpp>

namespace tower::player {
using namespace tower::entity;
using namespace tower::world;

class Player : public Entity {
public:
    explicit Player(EntityType type);

    static boost::asio::awaitable<std::shared_ptr<Player>> load(std::string_view character_name);
    static std::shared_ptr<Player> create(EntityType type);

    void tick(Subworld& subworld) override {}

    std::shared_ptr<Node> pivot {std::make_shared<Node>()};
    Inventory inventory {};
    Stat stat {};

private:
    uint64_t _character_id {0};
    std::string _name {};
};
}
