#pragma once

#include <tower/entity/entity.hpp>
#include <tower/system/timer.hpp>

#include <chrono>

namespace tower::entity {
using namespace std::chrono;

class EntitySpawner {
public:
    void tick();
    void spawn();

    EntityType entity_type {EntityType::NONE};
    glm::vec3 spawn_position {};
    seconds spawn_interval {10s};
    uint16_t max_entities_count {1};

    boost::signals2::signal<void(std::shared_ptr<Entity>)> spawned {};

private:
    steady_clock::time_point _last_spawned {};
    uint16_t _current_entities_count {0};
};
}