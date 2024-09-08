#pragma once

#include <tower/entity/entity.hpp>
#include <tower/world/tile_map.hpp>
#include <tower/world/collision/collision_object.hpp>

#include <unordered_set>

namespace tower::world {
using namespace tower::entity;

class Subworld {
public:
    explicit Subworld(std::string_view tile_map_name);

    void tick();

    void add_entity(const std::shared_ptr<Entity>& entity);
    void remove_entity(const std::shared_ptr<Entity>& entity);

    void add_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void remove_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void add_collision_area(const std::shared_ptr<CollisionObject>& area);
    void remove_collision_area(uint32_t area_id);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const std::shared_ptr<CollisionObject>& collider);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const CollisionShape* target_shape, uint32_t mask);

    const std::unordered_map<uint32_t, std::shared_ptr<Entity>>& get_entities() const { return _entities; }
    const TileMap& get_tilemap() const { return _tile_map; }
    glm::uvec2 get_size() const { return _tile_map.get_size(); }

private:
    std::unordered_map<uint32_t, std::shared_ptr<Entity>> _entities {};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_objects {};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_areas {};
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> _contacts {};
    TileMap _tile_map;
};
}
