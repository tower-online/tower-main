#include <tower/world/subworld.hpp>

namespace tower::world {
Subworld::Subworld(Grid<bool>&& obstacles_grid)
    : _obstacles_grid {std::move(obstacles_grid)} {}

void Subworld::tick() {
    // Move entities
    for (auto& [_, entity] : _entities) {
        //TODO: How to check if movable without hardcoding?
        if (entity->state_machine.get_current_state_name() == "Attacking") continue;

        //TODO: Pull out if entity is already in collider
        glm::vec3 next_position;
        if (entity->movement_mode == Entity::MovementMode::FORWARD) {
            if (all(epsilonEqual(entity->target_direction, zero3, 1e-5f))) continue;

            // TODO: If target position is blocked, try to approach as close as possible.
            next_position = entity->position + entity->target_direction * entity->movement_speed_base;
        } else if (entity->movement_mode == Entity::MovementMode::TARGET) {
            const auto target_distance {distance(entity->position, entity->target_position)};

            if (glm::epsilonEqual(target_distance, 0.0f, 1e-5f)) {
                entity->target_direction = zero3;
                continue;
            }

            entity->target_direction = normalize(entity->target_position - entity->position);
            if (target_distance <= entity->movement_speed_base) {
                next_position = entity->target_position;
            } else {
                next_position = entity->position + entity->target_direction * entity->movement_speed_base;
            }
        }

        const Point p {next_position.z, next_position.x};
        if (!_obstacles_grid.is_inside(p) || _obstacles_grid.at(p)) continue;
        entity->position = next_position;
    }

    // Update contacts
    for (const auto& [_, area] : _collision_areas) {
        for (const auto& [_, body] : _collision_objects) {
            if (area->is_colliding(body)) {
                if (_contacts[area->node_id].contains(body->node_id)) {
                    area->body_staying(body);
                } else {
                    area->body_entered(body);
                    _contacts[area->node_id].insert(body->node_id);
                }
            } else if (_contacts[area->node_id].contains(body->node_id)) {
                area->body_exited(body);
                _contacts[area->node_id].erase(body->node_id);
            }
        }
    }
}

void Subworld::add_entity(const std::shared_ptr<Entity>& entity) {
    add_collision_objects_from_tree(entity);
    _entities[entity->entity_id] = entity;
}

void Subworld::remove_entity(const std::shared_ptr<Entity>& entity) {
    remove_collision_objects_from_tree(entity);
    _entities.erase(entity->entity_id);
}

void Subworld::add_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_children()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            _collision_objects[object->node_id] = object;
        }
        add_collision_objects_from_tree(child);
    }
}

void Subworld::remove_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_children()) {
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
Subworld::get_collisions(const std::shared_ptr<CollisionObject>& collider) const {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, other] : _collision_objects) {
        if (collider->is_colliding(other)) continue;

        collisions.push_back(other);
    }

    return collisions;
}

std::vector<std::shared_ptr<CollisionObject>> Subworld::get_collisions(const CollisionShape* target_shape,
    const uint32_t mask) const {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, c] : _collision_objects) {
        if (!(mask & c->layer)) continue;

        if (!c->shape->is_colliding(target_shape)) continue;
        collisions.push_back(c);
    }

    return collisions;
}
}
