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
    explicit Player(EntityType type, uint32_t owner_id);

    static boost::asio::awaitable<std::shared_ptr<Player>> load(
        boost::mysql::pooled_connection& conn, std::string_view character_name, uint32_t owner_id);
    static std::shared_ptr<Player> create(EntityType type, uint32_t owner_id);

    void tick(network::Zone*) override {}

    flatbuffers::Offset<network::packet::PlayerData> write_player_info(flatbuffers::FlatBufferBuilder& builder) const;

    std::string_view name() const { return _name; }

    const uint32_t owner_id;

    Inventory inventory {};
    Stats stats {};

private:
    std::string _name {};
    uint32_t _character_id {};
};
}
