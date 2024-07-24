#include <spire/world/world.hpp>

namespace spire::world {
World::World() {}

void World::add_collider(const std::shared_ptr<Collider>& collider) {
    _colliders.insert_or_assign(collider->id, collider);
}

void World::remove_collider(const uint32_t collider_id) {
    _colliders.erase(collider_id);
}

void World::add_colliders_with_traverse(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto collider = std::dynamic_pointer_cast<Collider>(child)) {
            add_collider(collider);
        }
        add_colliders_with_traverse(child);
    }
}

void World::remove_colliders_with_traverse(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto collider = std::dynamic_pointer_cast<Collider>(child)) {
            remove_collider(collider->id);
        }
        remove_colliders_with_traverse(child);
    }
}

std::vector<std::shared_ptr<Collider>> World::get_collisions(std::shared_ptr<Collider>& collider) {
    std::vector<std::shared_ptr<Collider>> collisions {};
    for (auto& [_, c] : _colliders) {
        if (collider == c) continue;
        if (!(collider->mask & c->layer)) continue;

        if (collider->is_colliding(c)) {
            collisions.push_back(c);
        }
    }

    return collisions;
}
}
