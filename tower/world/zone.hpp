#pragma once

#include <tower/net/client_room.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/world/entity.hpp>
#include <tower/world/node.hpp>
#include <tower/world/tile_map.hpp>
#include <tower/world/collision/collision_object.hpp>

#include <chrono>
#include <unordered_map>


namespace tower::world {
using namespace std::chrono;
using namespace net::packet;

struct Portal : Node {
    Portal(const uint32_t target_zone_id, const size_t target_portal_index)
        : Node {}, target_zone_id {target_zone_id}, target_portal_index {target_portal_index} {}

    const uint32_t target_zone_id;
    const size_t target_portal_index;
};

class Zone {
    constexpr static auto TICK_INTERVAL = 100ms;

public:
    explicit Zone(std::string_view tile_map_name);
    ~Zone();

    void start();
    void stop();

    void add_client_deferred(std::shared_ptr<net::Client>&& client, size_t portal_index = 0);
    void remove_client_deferred(std::shared_ptr<net::Client> client);

private:
    void tick();

    void handle_packet(std::shared_ptr<net::Packet>&& packet);
    void hanlde_player_movement(std::shared_ptr<net::Client>&& client, const PlayerMovement* movement);
    void hanlde_entity_melee_attack(std::shared_ptr<net::Client>&& client, const EntityMeleeAttack* attack);

    // Physics
    void add_collision_object(const std::shared_ptr<CollisionObject>& object);
    void remove_collision_object(uint32_t collider_id);
    void add_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void remove_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(std::shared_ptr<CollisionObject>& collider);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const CollisionShape* target_shape, uint32_t mask);

private:
    // Network
    net::ClientRoom _client_room;
    std::shared_ptr<EventListener<std::shared_ptr<net::Client>>> _on_client_disconnected;

    // Objects
    std::shared_ptr<Node> _root {std::make_shared<Node>()};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_objects {};
    TileMap _tile_map;
    std::vector<Portal> _portals {};
    std::unordered_map<uint32_t, std::shared_ptr<Entity>> _entities {};

    // Jobs
    std::atomic<bool> _is_running {false};
    std::thread _worker_thread;
    ConcurrentQueue<std::function<void()>> _jobs {};
};
}
