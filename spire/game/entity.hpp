#pragma once

#include <spire/world/node.hpp>

#include <atomic>

namespace spire::game {
class Entity : public world::Node {
public:
    Entity();

    static uint32_t generate_entity_id();

    const uint32_t entity_id;
    glm::vec2 target_direction {};
};

inline Entity::Entity()
    : entity_id(generate_entity_id()) {}

inline uint32_t Entity::generate_entity_id() {
    static std::atomic<uint32_t> id_generator {0};

    return ++id_generator;
}
}
