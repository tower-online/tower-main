#pragma once

#include <tower/network/packet.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/network/server_shared_state.hpp>
#include <tower/world/subworld.hpp>
#include <tower/entity/entity.hpp>

#include <chrono>
#include <unordered_map>

namespace tower::network {
using namespace std::chrono;
using namespace tower::network::packet;

class Zone {
    constexpr static auto TICK_INTERVAL = 100ms;

public:
    Zone(uint32_t zone_id, boost::asio::any_io_executor& executor, const std::shared_ptr<ServerSharedState>& shared_state);
    ~Zone();

    static std::unique_ptr<Zone> create(uint32_t zone_id, boost::asio::any_io_executor& executor,
        std::string_view zone_data_file, const std::shared_ptr<ServerSharedState>& shared_state);

    void start();
    void stop();

    void add_client_deferred(const std::shared_ptr<Client>& client);
    void remove_client_deferred(const std::shared_ptr<Client>& client);
    void handle_packet_deferred(std::shared_ptr<Packet>&& packet);

    void broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, uint32_t except = 0);

    boost::asio::strand<boost::asio::any_io_executor>& strand() { return _strand; };
    world::Subworld* subworld() { return _subworld.get(); }

private:
    void tick();

    void handle_packet(std::shared_ptr<Packet>&& packet);
    void handle_player_movement(std::shared_ptr<Client>&& client, const PlayerMovement* movement);
    void handle_skill_melee_attack(std::shared_ptr<Client>&& client, const SkillMeleeAttack* attack);

public:
    const uint32_t zone_id;

private:
    std::atomic<bool> _is_running {false};

    boost::asio::strand<boost::asio::any_io_executor> _strand;
    steady_clock::time_point _last_tick;
    std::unique_ptr<world::Subworld> _subworld;

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};

    std::shared_ptr<ServerSharedState> _shared_state;
};
}
