#pragma once

#include <boost/signals2.hpp>
#include <tower/network/packet/entity_types.hpp>
#include <tower/world/node.hpp>
#include <tower/system/state_machine.hpp>

#include <atomic>

namespace tower::network {
class Zone;
}

namespace tower::entity {
namespace signal = boost::signals2;
using network::packet::EntityType;
using network::packet::EntityResourceType;
using network::packet::EntityResourceChangeMode;


struct EntityResource {
    int32_t max_health {0};
    int32_t health {0};

    signal::signal<void(EntityResourceChangeMode, int32_t)> health_changed;

    void change_health(EntityResourceChangeMode mode, int32_t amount);
};


class Entity : public world::Node {
public:
    explicit Entity(EntityType entity_type);

    static uint32_t generate_entity_id();

    virtual void tick(network::Zone* zone) = 0;

    const uint32_t entity_id;
    const EntityType entity_type;

    StateMachine state_machine {};
    EntityResource resource;

    glm::vec3 target_direction {};
    float movement_speed_base {0};

private:
    static void modify_resource_internal(EntityResourceChangeMode mode, uint32_t amount, uint32_t& target,
        uint32_t target_max);
};


inline void EntityResource::change_health(EntityResourceChangeMode mode, int32_t amount) {
    if (mode == EntityResourceChangeMode::ADD) {
        health = std::clamp(health + amount, 0, max_health);
        health_changed(mode, amount);
    } else if (mode == EntityResourceChangeMode::SET) {
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

inline static std::unordered_map<std::string, EntityType> entity_types_map {
    {"HUMAN", EntityType::HUMAN},
};
}
