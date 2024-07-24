#include <spire/game/equipment/equipment.hpp>
#include <spire/game/equipment/fist.hpp>
#include <spire/game/player/player.hpp>
#include <spire/world/collision/rectangle_collider.hpp>

#include "spdlog/spdlog.h"

namespace spire::game::player {
void Player::build() {
    const auto body_collider = std::make_shared<RectangleCollider>(
        glm::vec2 {0, 4}, glm::vec2 {7, 12}, static_cast<uint32_t>(ColliderLayer::ENTITIES), 0);
    add_child(body_collider);

    auto weapon_fist = std::make_shared<Fist>();
    auto a = std::static_pointer_cast<Equipment>(weapon_fist);
    inventory.set_main_weapon(a);
    add_child(weapon_fist->attack_collider);
}
}
