#pragma once

#include <tower/net/packet.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/world/node.hpp>
#include <tower/world/tile_map.hpp>
#include <tower/world/collision/collision_object.hpp>
#include <tower/world/entity/entity.hpp>

#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace tower::world {
using namespace std::chrono;
using namespace net::packet;

class Zone {
    constexpr static auto TICK_INTERVAL = 125ms;

public:
    Zone(uint32_t zone_id, std::string_view tile_map_name);
    ~Zone();

    void handle_packet_deferred(std::shared_ptr<net::Packet>&& packet);

    void start();
    void stop();

    void add_client_deferred(std::shared_ptr<net::Client>&& client);
    void remove_client_deferred(std::shared_ptr<net::Client> client);


private:
    void tick();

    void broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, uint32_t except = 0);
    void handle_packet(std::shared_ptr<net::Packet>&& packet);
    void hanlde_player_movement(std::shared_ptr<net::Client>&& client, const PlayerMovement* movement);
    void handle_player_enter_zone(std::shared_ptr<net::Client>&& client, const PlayerEnterZone* enter);
    void hanlde_entity_melee_attack(std::shared_ptr<net::Client>&& client, const EntityMeleeAttack* attack);

    // Physics
    void add_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void remove_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void add_collision_area(const std::shared_ptr<CollisionObject>& area);
    void remove_collision_area(uint32_t area_id);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const std::shared_ptr<CollisionObject>& collider);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const CollisionShape* target_shape, uint32_t mask);

public:
    const uint32_t zone_id;
    const std::string zone_name;

private:
    // Network
    std::unordered_map<uint32_t, std::shared_ptr<net::Client>> _clients {};
    std::shared_ptr<EventListener<std::shared_ptr<net::Client>>> _on_client_disconnected;

    // Objects
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_objects {};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_areas {};
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> _contacts {};
    std::unordered_map<uint32_t, std::shared_ptr<Entity>> _entities {};
    TileMap _tile_map;

    // Jobs
    std::atomic<bool> _is_running {false};
    std::thread _worker_thread;
    ConcurrentQueue<std::function<void()>> _jobs {};
};
}
