#include <spire/world/world.hpp>
#include <spdlog/spdlog.h>

namespace spire::world {
World::World() {}

void World::add_collision_object(const std::shared_ptr<CollisionObject>& object) {
    _collision_objects.insert_or_assign(object->id, object);
}

void World::remove_collision_object(const uint32_t collider_id) {
    _collision_objects.erase(collider_id);
}

void World::add_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            add_collision_object(object);
        }
        add_collision_objects_from_tree(child);
    }
}

void World::remove_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            remove_collision_object(object->id);
        }
        remove_collision_objects_from_tree(child);
    }
}

std::vector<std::shared_ptr<CollisionObject>> World::get_collisions(std::shared_ptr<CollisionObject>& collider) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, c] : _collision_objects) {
        if (collider == c) continue;
        if (!(collider->mask & c->layer)) continue;

        if (!collider->shape->is_colliding(c->shape)) continue;
        collisions.push_back(c);
    }

    return collisions;
}

std::vector<std::shared_ptr<CollisionObject>> World::get_collisions(const CollisionShape* target_shape, const uint32_t mask) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, c] : _collision_objects) {
        if (!(mask & c->layer)) continue;

        if (!c->shape->is_colliding(target_shape)) continue;
        collisions.push_back(c);
    }

    return collisions;
}
}
