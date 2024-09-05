#include <tower/item/equipment/fist.hpp>
#include <tower/network/db.hpp>
#include <tower/player/player.hpp>
#include <tower/world/collision/collision_object.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>

namespace tower::player {
Player::Player(const EntityType type)
    : Entity {type} {}

boost::asio::awaitable<std::shared_ptr<Player>> Player::load(std::string_view character_name) {
    auto conn = co_await DB::get_connection();
    auto [ec, statement] = co_await conn->async_prepare_statement(
        "SELECT c.id, c.name, c.race, c.level, c.exp"
        "FROM characters c "
        "JOIN character_stats st ON c.id = st.character_id "
        "JOIN character_skills sk ON c.id = st.character_id "
        "WHERE c.name = ?",
        as_tuple(boost::asio::use_awaitable));
    if (ec) co_return nullptr;

    boost::mysql::results result;
    if (const auto [ec] = co_await conn->async_execute(
        statement.bind(character_name), result, as_tuple(boost::asio::use_awaitable)); ec || result.rows().empty()) {
        co_return nullptr;
    }

    const auto r = result.rows()[0];
    const auto character_id = r.at(1).as_uint64();
    const auto name = r.at(1).as_string();
    const auto race = r.at(2).as_string();

    if (!entity_types_map.contains(race)) co_return nullptr;

    auto player = create(entity_types_map[race]);
    player->_character_id = character_id;
    player->_name = name;
    player->stat.

    co_return player;
}

std::shared_ptr<Player> Player::create(EntityType type) {
    auto player = std::make_shared<Player>(type);
    player->add_child(player->pivot);

    //TODO: Read values from file
    player->movement_speed_base = 0.1f;
    player->resource.max_health = 100.0f;
    player->resource.health = player->resource.max_health;

    const auto body_collider = CollisionObject::create(
        std::make_shared<RectangleCollisionShape>(glm::vec2 {12, 12}),
        static_cast<uint32_t>(ColliderLayer::ENTITIES | ColliderLayer::PLAYERS),
        0
    );
    player->add_child(body_collider);

    const auto weapon_offset = std::make_shared<Node>(glm::vec2 {12, 0}, 0);
    player->pivot->add_child(weapon_offset);

    auto fist = std::make_shared<Fist>();
    weapon_offset->add_child(fist->attack_shape);
    player->inventory.set_main_weapon(std::move(fist));

    return player;
}
}
