#pragma once

#include <boost/signals2.hpp>
#include <tower/net/packet/entity_types.hpp>
#include <tower/world/node.hpp>

#include <atomic>

namespace tower::world {
namespace signal = boost::signals2;
using net::packet::EntityType;
using net::packet::EntityResourceType;
using net::packet::EntityResourceModifyMode;

class Subworld;

struct EntityResource {
    int32_t max_health {0};
    int32_t health {0};

    signal::signal<void(EntityResourceModifyMode, int32_t)> health_changed;

    void change_health(EntityResourceModifyMode mode, int32_t amount);
};


class Entity : public Node {
public:
    explicit Entity(EntityType entity_type);

    static uint32_t generate_entity_id();
    void modify_resource(EntityResourceType type, EntityResourceModifyMode mode, uint32_t amount);

    virtual void tick(Subworld& subworld) = 0;

    const uint32_t entity_id;
    const EntityType entity_type;

    glm::vec2 target_direction {};
    float movement_speed_base {0};

    EntityResource resource;

private:
    static void modify_resource_internal(EntityResourceModifyMode mode, uint32_t amount, uint32_t& target,
        uint32_t target_max);
};


inline void EntityResource::change_health(EntityResourceModifyMode mode, int32_t amount) {
    if (mode == EntityResourceModifyMode::ARITHMETIC) {
        health = std::clamp(health + amount, 0, max_health);
        health_changed(mode, amount);
    } else if (mode == EntityResourceModifyMode::SET) {
        const auto health_old = health;
        health = std::clamp(amount, 0, max_health);
        health_changed(mode, health - health_old);
    }
}

inline Entity::Entity(const EntityType entity_type)
    : entity_id {generate_entity_id()}, entity_type {entity_type} {}

//TODO: Save/Load id_generator for server start/stop
inline uint32_t Entity::generate_entity_id() {
    static std::atomic<uint32_t> id_generator {0};

    return ++id_generator;
}
}
