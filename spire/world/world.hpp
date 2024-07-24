#pragma once

#include <spire/world/node.hpp>
#include <spire/world/collision/collider.hpp>

#include <unordered_map>

namespace spire::world {
class World {
public:
    World();

    void add_collider(const std::shared_ptr<Collider>& collider);
    void remove_collider(uint32_t collider_id);
    void add_colliders_with_traverse(const std::shared_ptr<Node>& node);
    void remove_colliders_with_traverse(const std::shared_ptr<Node>& node);
    std::vector<std::shared_ptr<Collider>> get_collisions(std::shared_ptr<Collider>& collider);

    std::shared_ptr<Node> get_root() const { return _root->shared_from_this(); }

private:
    std::shared_ptr<Node> _root {std::make_shared<Node>()};
    std::unordered_map<uint32_t, std::shared_ptr<Collider>> _colliders;
};
}
