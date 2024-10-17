#include <tower/entity/entity_spawner.hpp>
#include <tower/entity/simple_monster.hpp>
#include <tower/system/random.hpp>

namespace tower::entity {

void EntitySpawner::tick() {
    if (steady_clock::now() < _last_spawned + spawn_interval) return;
    spawn();
}

void EntitySpawner::spawn() {
    _last_spawned = steady_clock::now();
    if (_current_entities_count >= max_entities_count) return;
    if (entity_type == EntityType::NONE) return;

    std::shared_ptr<Entity> entity;
    if (entity_type == EntityType::SIMPLE_MONSTER) {
        entity = SimpleMonster::create();
    } else {
        return;
    }

    entity->position = spawn_position +
        glm::vec3 {random_range(-0.5f, 0.5f), 0, random_range(-0.5f, 0.5f)};
    entity->rotation = random_range(0.0f, deg2rad(360));

    entity->dead.connect([this](uint32_t, std::shared_ptr<Entity>) { _current_entities_count -= 1;});

    spawned(entity);
}
}
