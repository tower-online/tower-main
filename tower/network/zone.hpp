#pragma once

#include <tower/network/packet.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/world/subworld.hpp>

#include <chrono>
#include <unordered_map>

namespace tower::network {
using namespace std::chrono;
using namespace tower::network::packet;

class Zone {
    struct ClientEntry;

    constexpr static auto TICK_INTERVAL = 100ms;

public:
    Zone(uint32_t zone_id, boost::asio::any_io_executor& executor);
    ~Zone();

    void handle_packet_deferred(std::shared_ptr<Packet>&& packet);

    void init(std::string_view tile_map);
    void start();
    void stop();

    void add_client_deferred(const std::shared_ptr<Client>& client);
    void remove_client_deferred(const std::shared_ptr<Client>& client);

private:
    void tick();

    void broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, uint32_t except = 0);
    void handle_packet(std::shared_ptr<Packet>&& packet);
    void handle_player_movement(std::shared_ptr<Client>&& client, const PlayerMovement* movement);
    void handle_skill_melee_attack(std::shared_ptr<Client>&& client, const SkillMeleeAttack* attack);

public:
    const uint32_t zone_id;

private:
    std::atomic<bool> _is_running {false};

    boost::asio::strand<boost::asio::any_io_executor> _strand;
    steady_clock::time_point _last_tick;

    std::unordered_map<uint32_t, std::unique_ptr<ClientEntry>> _client_entries {};

    std::unique_ptr<world::Subworld> _subworld;
};

struct Zone::ClientEntry {
    ClientEntry(const std::shared_ptr<Client>& client, boost::signals2::connection&& on_disconnected)
        : client {client}, _on_disconnected {std::move(on_disconnected)} {}
    std::shared_ptr<Client> client;

private:
    boost::signals2::connection _on_disconnected;
};
}
