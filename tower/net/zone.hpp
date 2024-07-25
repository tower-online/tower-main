#pragma once

#include <tower/net/client.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/world/world.hpp>

#include <chrono>

namespace tower::net {
using namespace std::chrono;
using namespace world;

struct Portal;

class Zone : public std::enable_shared_from_this<Zone> {
    constexpr static auto TICK_INTERVAL = 100ms;

public:
    ~Zone();

    void start();
    void stop();
    void add_client(std::shared_ptr<Client> client);
    void remove_client(uint32_t client_id);
    void broadcast_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer, uint32_t except = 0);

    void add_portal(const std::shared_ptr<Portal>& portal);

private:
    void handle_packet(std::shared_ptr<Packet>&& packet);
    void hanlde_entity_movement(std::shared_ptr<Client>&& client, const packet::EntityMovement* movement);
    void hanlde_entity_action(std::shared_ptr<Client>&& client, const packet::EntityAction* action);
    void tick();

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    World _world {};
    std::vector<std::shared_ptr<Portal>> _portals {};

    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;
    std::atomic<bool> _is_running {false};
};

struct Portal : Node {
    Portal(const std::shared_ptr<Zone>& target_zone, const size_t target_portal_idx)
        : target_zone {target_zone}, target_portal_idx {target_portal_idx} {}

    std::weak_ptr<Zone> target_zone;
    size_t target_portal_idx;
    bool is_enabled {true};
};
}
