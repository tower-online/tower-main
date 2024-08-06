#include <tower/world/subworld.hpp>

namespace tower::world {
Subworld::Subworld(std::string_view tile_map_name)
    : _tile_map {TileMap::load_tile_map(tile_map_name)} {}

void Subworld::tick() {
    // Move entities
    for (auto& [_, entity] : _entities) {
        // Check if tile is blocked and move
        //TODO: Pull out if entity is already in collider
        if (entity->target_direction != glm::vec2 {0.0f, 0.0f}) {
            const auto target_position = entity->position + entity->target_direction * entity->movement_speed_base;
            if (Tile tile; _tile_map.try_at(target_position, tile) && tile.state != TileState::BLOCKED) {
                entity->position = target_position;
            }
        }
    }

    // Update contacts
    for (const auto& [_, area] : _collision_areas) {
        for (const auto& [_, body] : _collision_objects) {
            if (area->is_colliding(body)) {
                if (_contacts[area->node_id].contains(body->node_id)) {
                    area->body_staying.notify(body);
                } else {
                    area->body_entered.notify(body);
                    _contacts[area->node_id].insert(body->node_id);
                }
            } else if (_contacts[area->node_id].contains(body->node_id)) {
                area->body_exited.notify(body);
                _contacts[area->node_id].erase(body->node_id);
            }
        }
    }

    for (auto& [_, entity] : _entities) {
        entity->tick(*this);
    }
}

void Subworld::add_entity(const std::shared_ptr<Entity>& entity) {
    add_collision_objects_from_tree(entity);
    _entities[entity->entity_id] = entity;
}

void Subworld::add_entity(std::shared_ptr<Entity>&& entity) {
    add_collision_objects_from_tree(entity);
    _entities[entity->entity_id] = std::move(entity);
}

void Subworld::remove_entity(std::shared_ptr<Entity>&& entity) {
    remove_collision_objects_from_tree(entity);
    _entities.erase(entity->entity_id);
}

void Subworld::remove_entity(const std::shared_ptr<Entity>& entity) {
    remove_collision_objects_from_tree(entity);
    _entities.erase(entity->entity_id);
}

void Subworld::add_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            _collision_objects[object->node_id] = object;
        }
        add_collision_objects_from_tree(child);
    }
}

void Subworld::remove_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            _collision_objects.erase(object->node_id);
        }
        remove_collision_objects_from_tree(child);
    }
}

void Subworld::add_collision_area(const std::shared_ptr<CollisionObject>& area) {
    _collision_areas[area->node_id] = area;
    _contacts[area->node_id] = {};
}

void Subworld::remove_collision_area(const uint32_t area_id) {
    _collision_areas.erase(area_id);
    _contacts.erase(area_id);
}

std::vector<std::shared_ptr<CollisionObject>>
Subworld::get_collisions(const std::shared_ptr<CollisionObject>& collider) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, other] : _collision_objects) {
        if (collider->is_colliding(other)) continue;

        collisions.push_back(other);
    }

    return collisions;
}

std::vector<std::shared_ptr<CollisionObject>> Subworld::get_collisions(const CollisionShape* target_shape,
    const uint32_t mask) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, c] : _collision_objects) {
        if (!(mask & c->layer)) continue;

        if (!c->shape->is_colliding(target_shape)) continue;
        collisions.push_back(c);
    }

    return collisions;
}
}
