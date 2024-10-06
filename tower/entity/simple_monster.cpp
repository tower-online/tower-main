#include <tower/entity/simple_monster.hpp>
#include <tower/physics/collision_object.hpp>
#include <tower/physics/cube_collision_shape.hpp>
#include <tower/player/player.hpp>
#include <tower/world/subworld.hpp>

namespace tower::entity {

void SimpleMonster::tick(world::Subworld& subworld) {
    // if (_state == State::IDLE) {
    //     const auto targets {subworld.get_collisions(
    //         _detection_area.get(), std::to_underlying(physics::ColliderLayer::PLAYERS))};
    //
    //     for (const auto& target : targets) {
    //         player::Player* player;
    //         if (auto entity {dynamic_cast<Entity*>(target->get_root())}; entity) {
    //             if (player = dynamic_cast<player::Player*>(entity); !player) continue;
    //         } else continue;
    //
    //         // Find path and start follow
    //
    //         _state = State::CHASING;
    //         break;
    //     }
    // } else if (_state == State::CHASING) {
    //     // Follow path
    // }
}

std::shared_ptr<SimpleMonster> SimpleMonster::create() {
    using namespace physics;

    // auto monster {std::make_shared<SimpleMonster>()};
    //
    // const auto body_collider = CollisionObject::create(
    //     std::make_shared<CubeCollisionShape>(glm::vec3 {0.5, 1, 0.5}),
    //     std::to_underlying(ColliderLayer::ENTITIES),
    //     0
    // );
    // monster->add_child(body_collider);
    //
    // monster->movement_speed_base = 0.1f;
    // monster->resource.max_health = 10.0f;
    // monster->resource.health = monster->resource.max_health;
    //
    // monster->_detection_area = std::make_shared<SphereCollisionShape>(4);
    // monster->add_child(monster->_detection_area);
    //
    // monster->_weapon = item::equipment::Fist::create();
    // //TODO: Shape position needs to be added to children
    // monster->add_child(monster->_weapon->melee_attack_shape());
    //
    // return monster;
    return {};
}
}
