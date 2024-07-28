#pragma once

#include <tower/world/tile_map.hpp>
#include <tower/world/node.hpp>
#include <tower/world/collision/collision_object.hpp>

#include <unordered_map>

namespace tower::world {
class World {
public:
    World();

    void add_collision_object(const std::shared_ptr<CollisionObject>& object);
    void remove_collision_object(uint32_t collider_id);
    void add_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void remove_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(std::shared_ptr<CollisionObject>& collider);
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const CollisionShape* target_shape, uint32_t mask);

    std::shared_ptr<Node>& get_root() { return _root; }

private:
    std::shared_ptr<Node> _root {std::make_shared<Node>()};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_objects;
    TileMap _grid;
};


}
