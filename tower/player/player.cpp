#include <spdlog/spdlog.h>
#include <tower/item/equipment/weapon/fist.hpp>
#include <tower/player/player.hpp>
#include <tower/player/state/idle_state.hpp>
#include <tower/player/state/attacking_state.hpp>
#include <tower/physics/collision_object.hpp>
#include <tower/physics/cube_collision_shape.hpp>

namespace tower::player {
Player::Player(const EntityType type)
    : Entity {type} {}

boost::asio::awaitable<std::shared_ptr<Player>> Player::load(boost::mysql::pooled_connection& conn, std::string_view character_name) {
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
    //TODO: Use static result
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
    using namespace physics;

    auto player = std::make_shared<Player>(type);

    // Init states
    {
        auto& state_machine {player->state_machine};
        auto idle {std::make_unique<IdleState>()};
        auto attacking {std::make_unique<AttackingState>()};

        idle->add_transition(attacking->get_name());
        attacking->add_transition(idle->get_name());

        const auto idle_state_name {idle->get_name()};
        state_machine.add_state(std::move(idle));
        state_machine.add_state(std::move(attacking));
        state_machine.set_initial_state(idle_state_name);
    }

    //TODO: Read values from file
    player->movement_speed_base = 0.25f;
    player->resource.max_health = 100.0f;
    player->resource.health = player->resource.max_health;

    const auto body_collider = CollisionObject::create(
        std::make_shared<CubeCollisionShape>(glm::vec3 {0.5, 1, 0.5}),
        ColliderLayer::ENTITIES | ColliderLayer::PLAYERS,
        0
    );
    player->add_child(body_collider);

    const auto weapon_offset = std::make_shared<Node>(glm::vec3 {0, 0, 1}, 0);
    player->add_child(weapon_offset);

    //TODO: Read inventory items from DB
    auto fist = Fist::create();
    weapon_offset->add_child(fist->node);
    player->inventory.set_main_weapon(std::move(fist));

    return player;
}

flatbuffers::Offset<network::packet::PlayerData> Player::write_player_info(flatbuffers::FlatBufferBuilder& builder) const {
    using namespace network::packet;

    std::vector<PlayerResource> resources {};
    resources.emplace_back(EntityResourceType::HEALTH, resource.health, resource.max_health);

    std::vector<PlayerStat> optional_stats {};
    for (const auto& [_, stat] : stats.optionals) {
        optional_stats.emplace_back(stat.type(), stat.get());
    }
    const auto stats_offset = CreatePlayerStatsDirect(builder,
        stats.level.get(), stats.exp.get(), stats.str.get(), stats.mag.get(), stats.agi.get(), stats.con.get(),
        &optional_stats);

    return CreatePlayerDataDirect(builder, entity_type, name().data(), &resources, stats_offset);
}
}
