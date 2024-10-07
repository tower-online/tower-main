#pragma once

#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <tower/entity/entity.hpp>
#include <tower/network/packet/player_types.hpp>
#include <tower/player/inventory.hpp>
#include <tower/player/stat.hpp>

namespace tower::player {
using namespace tower::entity;
using namespace tower::world;

class Player : public Entity {
public:
    explicit Player(EntityType type);

    static boost::asio::awaitable<std::shared_ptr<Player>> load(boost::mysql::pooled_connection& conn, std::string_view character_name);
    static std::shared_ptr<Player> create(EntityType type);

    void tick(network::Zone*) override {}

    flatbuffers::Offset<network::packet::PlayerData> write_player_info(flatbuffers::FlatBufferBuilder& builder) const;

    uint32_t character_id() const { return _character_id; }
    std::string_view name() const { return _name; }

    std::shared_ptr<Node> pivot {std::make_shared<Node>()};
    Inventory inventory {};
    Stats stats {};

private:
    uint32_t _character_id {};
    std::string _name {};
};
}
