#pragma once

#include <tower/net/packet.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/world/subworld.hpp>

#include <chrono>
#include <unordered_map>

namespace tower::net {
using namespace std::chrono;
using namespace tower::net::packet;

class Zone {
    constexpr static auto TICK_INTERVAL = 125ms;

public:
    Zone(uint32_t zone_id, std::string_view tile_map_name);
    ~Zone();

    void handle_packet_deferred(std::shared_ptr<Packet>&& packet);

    void start();
    void stop();

    void add_client_deferred(std::shared_ptr<Client>&& client);
    void remove_client_deferred(std::shared_ptr<Client>&& client);

private:
    void tick();

    void broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, uint32_t except = 0);
    void handle_packet(std::shared_ptr<Packet>&& packet);
    void hanlde_player_movement(std::shared_ptr<Client>&& client, const PlayerMovement* movement);
    void handle_player_enter_zone(std::shared_ptr<Client>&& client, const PlayerEnterZone* enter);
    void hanlde_entity_melee_attack(std::shared_ptr<Client>&& client, const EntityMeleeAttack* attack);

public:
    const uint32_t zone_id;

private:
    std::atomic<bool> _is_running {false};

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::shared_ptr<EventListener<std::shared_ptr<Client>>> _on_client_disconnected;

    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;

    world::Subworld _subworld;
};
}
