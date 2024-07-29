#include <tower/game/equipment/fist.hpp>
#include <tower/game/player/player.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::game::player {
Player::Player()
    : Entity {EntityType::PLAYER_HUMAN} {}

std::shared_ptr<Player> Player::create() {
    auto player = std::make_shared<Player>();

    player->movement_speed_base = 10.0f;
    player->resource.max_health = 100.0f;
    player->resource.health = player->resource.max_health;

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
