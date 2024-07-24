#include <spire/game/equipment/fist.hpp>
#include <spire/game/player/player.hpp>
#include <spire/world/collision/rectangle_collision_shape.hpp>

namespace spire::game::player {
std::shared_ptr<Player> Player::create() {
    auto player = std::make_shared<Player>();

    auto body_collider = CollisionObject::create(
        std::make_shared<RectangleCollisionShape>(glm::vec2 {7, 12}),
        static_cast<uint32_t>(ColliderLayer::ENTITIES),
        0
    );
    body_collider->position = {0, 4};
    player->add_child(body_collider);

    auto fist = std::make_shared<Fist>();
    player->add_child(fist->attack_shape);
    player->inventory.set_main_weapon(std::move(fist));

    return player;
}
}
