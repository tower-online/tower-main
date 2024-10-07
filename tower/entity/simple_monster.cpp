#include <tower/entity/simple_monster.hpp>
#include <tower/physics/collision_object.hpp>
#include <tower/physics/cube_collision_shape.hpp>
#include <tower/player/player.hpp>
#include <tower/world/path_finder.hpp>
#include <tower/world/subworld.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/packet/entity_types.hpp>
#include <tower/skill/melee_attack.hpp>

#include <algorithm>

namespace tower::entity {
SimpleMonster::SimpleMonster()
    : Entity {EntityType::SIMPLE_MONSTER} {}

void SimpleMonster::tick(network::Zone* zone) {
    //TODO: `_chasing_target` is shared_ptr, holding the target even if the player has already left the zone. Fix.
    if (_state == State::IDLE) {
        // Check if any player is within the detection area
        auto targets {zone->subworld()->get_collisions(
            _detection_area.get(), std::to_underlying(physics::ColliderLayer::PLAYERS))};
        if (targets.empty()) return;

        // Chase the closest target
        const auto my_position {get_global_position()};
        std::sort(targets.begin(), targets.end(), [this, &my_position](const auto& t1, const auto& t2) {
            return distance(t1->get_global_position(), my_position) < distance(t2->get_global_position(), my_position);
        });

        const auto player {dynamic_cast<const player::Player*>(targets.at(0)->get_root().get())};
        if (!player) return;

        // Find path and start chasing
        _chasing_path = world::Pathfinder::find_path(zone->subworld()->obstacles_grid(),
           {position.z, position.x}, {player->position.z, player->position.x});
        if (_chasing_path.empty()) return;

        _chasing_started = steady_clock::now();
        _chasing_path_index = 0;
        _state = State::CHASING;
    } else if (_state == State::CHASING) {
        if (!_chasing_target) {
            _state = State::IDLE;
            return;
        }

        // If chasing lasted more than 3 secs, update path
        if (const auto now {steady_clock::now()}; now > _chasing_started + 3s) {
            _chasing_path = world::Pathfinder::find_path(zone->subworld()->obstacles_grid(),
            {position.z, position.x}, {_chasing_target->position.z, _chasing_target->position.x}
            );
            _chasing_path_index = 0;
            _chasing_started = now;

            if (_chasing_path.empty()) {
                _state = State::IDLE;
                return;
            }
        }

        const auto current_position {get_global_position()};

        // Arrived
        if (_chasing_path_index >= _chasing_path.size()) {
            // Attack if close enough
            if (distance(current_position, _chasing_target->position) < 0.5) {
                _state = State::ATTACKING;
                return;
            }

            target_direction = _chasing_target->position - current_position;
            return;
        }

        // Follow path
        const glm::vec3 next_position {
            _chasing_path.at(_chasing_path_index).c + 0.5, 0, _chasing_path.at(_chasing_path_index).r + 0.5};
        target_direction = next_position - current_position;

        if (distance(current_position, next_position) < 0.2) {
            _chasing_path_index += 1;
            return;
        }
    } else if (_state == State::ATTACKING) {
        if (!_chasing_target) {
            _state = State::IDLE;
            return;
        }

        //TODO: Set out of range of melee attack instead of hard-coded value
        if (distance(get_global_position(), _chasing_target->get_global_position()) > 0.5) {
            _state = State::CHASING;
            return;
        }

        // Attack
        skill::MeleeAttack::use(zone, this, _weapon.get());
    }
}

std::shared_ptr<SimpleMonster> SimpleMonster::create() {
    using namespace physics;

    auto monster {std::make_shared<SimpleMonster>()};
    
    const auto body_collider = CollisionObject::create(
        std::make_shared<CubeCollisionShape>(glm::vec3 {0.5, 1, 0.5}),
        std::to_underlying(ColliderLayer::ENTITIES),
        0
    );
    monster->add_child(body_collider);
    
    monster->movement_speed_base = 0.1f;
    monster->resource.max_health = 10.0f;
    monster->resource.health = monster->resource.max_health;
    
    monster->_detection_area = std::make_shared<SphereCollisionShape>(4);
    monster->add_child(monster->_detection_area);
    
    monster->_weapon = item::equipment::Fist::create();
    monster->add_child(monster->_weapon->node);
    
    return monster;
}
}
