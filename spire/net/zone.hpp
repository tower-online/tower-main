#pragma once

#include <spire/net/client.hpp>
#include <spire/net/packet/packet_base.hpp>
#include <spire/world/world.hpp>

#include <chrono>

namespace spire::net {
using namespace std::chrono;
using namespace world;

class Zone {
    constexpr static auto TICK_INTERVAL = 100ms;

public:
    ~Zone();

    void start();
    void stop();
    void add_client(std::shared_ptr<Client> client);
    void remove_client(uint32_t client_id);
    void broadcast_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer, uint32_t except = 0);

private:
    void handle_packet(std::shared_ptr<Packet>&& packet);
    void hanlde_entity_movement(std::shared_ptr<Client>&& client, const packet::EntityMovement* movement);
    void hanlde_entity_action(std::shared_ptr<Client>&& client, const packet::EntityAction* action);
    void tick();

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    World _world {};

    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;
    std::atomic<bool> _is_running {false};
};
}
