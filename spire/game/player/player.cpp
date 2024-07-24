#include <spire/game/equipment/fist.hpp>
#include <spire/game/player/player.hpp>
#include <spire/world/collision/rectangle_collision_shape.hpp>

#include "spdlog/spdlog.h"

namespace spire::game::player {
void Player::initialize() {
    std::shared_ptr<CollisionShape> body_collider_shape = std::make_shared<RectangleCollisionShape>(glm::vec2 {7, 12});
    const auto body_collider = std::make_shared<CollisionObject>(
        static_cast<uint32_t>(ColliderLayer::ENTITIES),
        0);
    body_collider->set_shape(body_collider_shape);
    body_collider->position = {0, 4};
    add_child(body_collider);

    auto fist = std::make_shared<Fist>();
    add_child(fist->attack_shape);
    inventory.set_main_weapon(std::move(fist));
}
}
