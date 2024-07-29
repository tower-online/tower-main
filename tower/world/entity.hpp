#pragma once

#include <tower/net/packet/entity_types.hpp>
#include <tower/system/event.hpp>
#include <tower/world/node.hpp>

#include <atomic>

namespace tower::world {
using net::packet::EntityType;
using net::packet::EntityResourceType;
using net::packet::EntityResourceModifyMode;

struct EntityResource {
    uint32_t max_health {0};
    uint32_t health {0};
};

class Entity : public Node {
public:
    explicit Entity(EntityType entity_type);

    static uint32_t generate_entity_id();
    void modify_resource(EntityResourceType type, EntityResourceModifyMode mode, uint32_t amount);

    const uint32_t entity_id;
    const EntityType entity_type;

    EntityResource resource;
    glm::vec2 target_direction {};
    float movement_speed_base {0};

    // <EntityResourceType, EntityResourceModifyMode, modifying_value, current_value>
    Event<EntityResourceType, EntityResourceModifyMode, uint32_t, uint32_t> resource_modified;

private:
    static void modify_resource_internal(EntityResourceModifyMode mode, uint32_t amount, uint32_t& target,
        uint32_t target_max);
};

inline Entity::Entity(const EntityType entity_type)
    : entity_id {generate_entity_id()}, entity_type {entity_type} {}

//TODO: Save/Load id_generator for server start/stop
inline uint32_t Entity::generate_entity_id() {
    static std::atomic<uint32_t> id_generator {0};

    return ++id_generator;
}

inline void Entity::modify_resource(const EntityResourceType type, const EntityResourceModifyMode mode,
    const uint32_t amount) {
    switch (type) {
    case EntityResourceType::HEALTH:
        modify_resource_internal(mode, amount, resource.health, resource.max_health);
        resource_modified.notify(type, mode, amount, resource.health);
        break;

    default:
        break;
    }
}

inline void Entity::modify_resource_internal(const EntityResourceModifyMode mode, const uint32_t amount,
    uint32_t& target, const uint32_t target_max) {
    if (mode == EntityResourceModifyMode::SET) {
        target = amount;
    } else if (mode == EntityResourceModifyMode::POSITIVE) {
        target = std::clamp(target + amount, 0u, target_max);
    } else if (mode == EntityResourceModifyMode::NEGATIVE) {
        target = std::clamp(target - amount, 0u, target_max);
    }
}
}
