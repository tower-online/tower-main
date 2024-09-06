#include <spdlog/spdlog.h>
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
        "SELECT id, name, race "
        "FROM characters "
        "WHERE name = ?",
        as_tuple(boost::asio::use_awaitable));
    if (ec) {
        spdlog::error("[Player] Error preparing statement1: {}", ec.message());
        co_return nullptr;
    }

    boost::mysql::results result;
    if (const auto [ec] = co_await conn->async_execute(
        statement.bind(character_name), result, as_tuple(boost::asio::use_awaitable)); ec) {
        spdlog::error("[Player] Error executing1: {}", ec.message());
        co_return nullptr;
    }

    if (result.rows().empty()) {
        co_return nullptr;
    }

    const auto r1 = result.rows()[0];
    uint32_t character_id;
    std::string_view name, race;

    try {
        character_id = r1.at(0).as_int64();
        name = r1.at(1).as_string();
        race = r1.at(2).as_string();
    } catch (const boost::mysql::bad_field_access& e) {
        spdlog::error("[Player] error loading: {}", e.what());
        co_return nullptr;
    }

    auto player = create(entity_types_map[race.data()]);
    player->_character_id = character_id;
    player->_name = name;

    std::string query {
        format_sql(
            conn->format_opts().value(),
            "SELECT * "
            "FROM character_stats "
            "WHERE character_id = {}",
            character_id
        )
    };
    if (const auto [ec] = co_await conn->async_execute(std::move(query), result, as_tuple(boost::asio::use_awaitable));
        ec) {
        spdlog::error("[Player] Error executing2: {}", ec.message());
        co_return nullptr;
    }

    if (result.rows().empty()) {
        co_return nullptr;
    }

    const auto r2 = result.rows()[0];
    auto& stats = player->stats;
    try {
        stats.level.set(static_cast<int16_t>(r2.at(1).as_int64()));
        stats.exp.set(static_cast<int32_t>(r2.at(2).as_int64()));

        stats.str.set(static_cast<int16_t>(r2.at(3).as_int64()));
        stats.mag.set(static_cast<int16_t>(r2.at(4).as_int64()));
        stats.agi.set(static_cast<int16_t>(r2.at(5).as_int64()));
        stats.con.set(static_cast<int16_t>(r2.at(6).as_int64()));

        if (!r2.at(7).is_null()) {
            stats.optionals.insert_or_assign(StatType::FAITH, Stat<int16_t> {StatType::FAITH});
            stats.optionals.at(StatType::FAITH).set(static_cast<int16_t>(r2.at(7).as_int64()));
        }
    } catch (const boost::mysql::bad_field_access& e) {
        spdlog::error("[Player] error loading: {}", e.what());
        co_return nullptr;
    }

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
