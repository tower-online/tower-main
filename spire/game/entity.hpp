#pragma once

#include <glm/vec2.hpp>

#include <atomic>

namespace spire::game {
class Entity {
public:
    Entity();
    virtual ~Entity() = default;

    static uint32_t generate_entity_id();

    const uint32_t entity_id;
    glm::vec2 position {};
};
}
