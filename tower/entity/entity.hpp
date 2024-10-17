#pragma once

#include <boost/signals2.hpp>
#include <tower/network/packet/entity_types.hpp>
#include <tower/world/world_object.hpp>
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

    void change_health(EntityResourceChangeMode mode, int32_t amount);
};


class Entity : public world::WorldObject {
public:
    enum class MovementMode {FORWARD, TARGET};

    explicit Entity(EntityType entity_type);

    virtual void tick(network::Zone* zone) = 0;

    void get_damage(const std::shared_ptr<Entity>& attacker, EntityResourceType type, int32_t amount);

public:
    const uint32_t entity_id;
    const EntityType entity_type;

    StateMachine state_machine {};
    EntityResource resource;

    boost::signals2::signal<void(uint32_t entity_id, std::shared_ptr<Entity> killer)> dead {};

    MovementMode movement_mode {MovementMode::FORWARD};
    glm::vec3 target_direction {};
    glm::vec3 target_position {};
    float movement_speed_base {0};

private:
    inline static std::atomic<uint32_t> _id_generator {0};
};


inline void EntityResource::change_health(const EntityResourceChangeMode mode, const int32_t amount) {
    if (mode == EntityResourceChangeMode::ADD) {
        health = std::clamp(health + amount, 0, max_health);
    } else if (mode == EntityResourceChangeMode::SET) {
        health = std::clamp(amount, 0, max_health);
    }
}

inline Entity::Entity(const EntityType entity_type)
    : entity_id {++_id_generator}, entity_type {entity_type} {}

inline void Entity::get_damage(const std::shared_ptr<Entity>& attacker,
    const EntityResourceType type, const int32_t amount) {
    if (type == EntityResourceType::HEALTH) {
        resource.change_health(EntityResourceChangeMode::ADD, -amount);

        if (resource.health == 0) {
            dead(entity_id, attacker);
        }
    }
}

inline static std::unordered_map<std::string, EntityType> entity_name_to_type {
    {"HUMAN", EntityType::HUMAN},
};
}
