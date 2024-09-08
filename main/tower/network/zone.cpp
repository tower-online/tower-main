#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <tower/entity/entity.hpp>
#include <tower/item/equipment/fist.hpp>
#include <tower/network/zone.hpp>
#include <tower/system/math.hpp>

#include <cmath>

namespace tower::network {
using namespace tower::player;

Zone::Zone(const uint32_t zone_id, boost::asio::any_io_executor& executor)
    : zone_id {zone_id}, _strand {make_strand(executor)} {}

Zone::~Zone() {
    stop();
}

void Zone::handle_packet_deferred(std::shared_ptr<Packet>&& packet) {
    co_spawn(_strand, [this, packet {std::move(packet)}]() mutable ->boost::asio::awaitable<void> {
        handle_packet(std::move(packet));
        co_return;
    }, boost::asio::detached);
}

void Zone::init(std::string_view tile_map) {
    _subworld = std::make_unique<Subworld>(tile_map);
}

void Zone::start() {
    if (_is_running.exchange(true)) return;

    post(_strand, [this] { tick(); });
}

void Zone::stop() {
    if (!_is_running.exchange(false)) return;

    //TODO: clear
}

void Zone::add_client_deferred(std::shared_ptr<Client>&& client) {
    co_spawn(_strand, [this, client = std::move(client)]()->boost::asio::awaitable<void> {
        if (_client_entries.empty()) start();

        auto entry = std::make_unique<ClientEntry>(
            client,
            client->disconnected.connect([this](const std::shared_ptr<Client>& c) {
                remove_client_deferred(c);
            }));
        _client_entries.insert_or_assign(entry->client->entry_id, std::move(entry));

        const auto& player = client->player;
        player->position = {0, 0};

        _subworld->add_entity(player);

        {
            flatbuffers::FlatBufferBuilder builder {128};
            const auto enter =
                CreatePlayerEnterZoneDirect(builder, zone_id, _subworld->get_tilemap().get_name().data());
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerEnterZone, enter.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Spawn every entity in the zone
        {
            std::vector<EntitySpawn> spawns {};
            for (const auto& [entity_id, entity] : _subworld->get_entities()) {
                if (entity_id == player->entity_id) continue;

                spawns.emplace_back(entity->entity_type, entity->entity_id,
                    Vector2 {entity->position.x, entity->position.y}, entity->rotation);
            }

            flatbuffers::FlatBufferBuilder builder {1024};
            const auto entity_spawns = CreateEntitySpawnsDirect(builder, &spawns);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, entity_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Notify other clients for new player spawn
        {
            std::vector<EntitySpawn> spawns {};
            spawns.emplace_back(player->entity_type, player->entity_id,
                Vector2 {player->position.x, player->position.y}, player->rotation);

            flatbuffers::FlatBufferBuilder builder {256};
            const auto spawn = CreateEntitySpawnsDirect(builder, &spawns);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, spawn.Union()));
            broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->entry_id);
        }

        co_return;
    }, boost::asio::detached);
}

void Zone::remove_client_deferred(const std::shared_ptr<Client>& client) {
    co_spawn(_strand, [this, client]()->boost::asio::awaitable<void> {
        _subworld->remove_entity(client->player);
        _client_entries.erase(client->entry_id);
        spdlog::info("[Zone] Removed Client({})", client->entry_id);

        // Broadcast EntityDespawn
        flatbuffers::FlatBufferBuilder builder {64};
        const auto entity_despawn = CreateEntityDespawn(builder, client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityDespawn, entity_despawn.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        if (_client_entries.empty()) {
            stop();
        }

        co_return;
    }, boost::asio::detached);
}

void Zone::tick() {
    if (steady_clock::now() < _last_tick + TICK_INTERVAL) {
        post(_strand, [this] { tick(); });
        return;
    }

    _subworld->tick();

    // Broadcast entities' transform
    {
        std::vector<EntityMovement> movements {};
        for (const auto& [_, entity] : _subworld->get_entities()) {
            movements.emplace_back(
                entity->entity_id,
                Vector2 {entity->position.x, entity->position.y},
                Vector2 {entity->target_direction.x, entity->target_direction.y}
            );
        }

        flatbuffers::FlatBufferBuilder builder {};
        const auto entity_movements = CreateEntityMovementsDirect(builder, &movements);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityMovements, entity_movements.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    _last_tick = steady_clock::now();
    post(_strand, [this] { tick(); });
}

void Zone::broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, const uint32_t except) {
    for (const auto& [id, entry] : _client_entries) {
        if (id == except) continue;
        entry->client->send_packet(buffer);
    }
}

void Zone::handle_packet(std::shared_ptr<Packet>&& packet) {
    if (!_client_entries.contains(packet->client->entry_id)) return;

    const auto packet_base = GetPacketBase(packet->buffer.data());
    if (!packet_base) {
        spdlog::warn("[Zone] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::PlayerMovement:
        handle_player_movement(std::move(packet->client), packet_base->packet_base_as<PlayerMovement>());
        break;

    case PacketType::EntityMeleeAttack:
        handle_entity_melee_attack(std::move(packet->client), packet_base->packet_base_as<EntityMeleeAttack>());
        break;

    default:
        break;
    }
}

void Zone::handle_player_movement(std::shared_ptr<Client>&& client, const PlayerMovement* movement) {
    glm::vec2 target_direction;
    if (const auto target_direction_ptr = movement->target_direction(); !target_direction_ptr) {
        return;
    } else {
        target_direction = {target_direction_ptr->x(), target_direction_ptr->y()};
    }

    if (std::isnan(target_direction.x) || std::isnan(target_direction.y)) {
        return;
    }
    if (target_direction != glm::vec2 {0.0f, 0.0f}) {
        target_direction = normalize(target_direction);
    }

    const auto& player = client->player;
    player->target_direction = target_direction;
    if (player->target_direction != glm::vec2 {0.0f, 0.0f}) {
        // player->pivot->rotation = glm::atan(player->target_direction.y / player->target_direction.x);
        player->pivot->rotation = direction_to_4way_angle(player->target_direction);
    }
}

void Zone::handle_entity_melee_attack(std::shared_ptr<Client>&& client, const EntityMeleeAttack* attack) {
    // Replicate player's melee attack
    {
        flatbuffers::FlatBufferBuilder builder {128};
        const auto attack_replication = CreateEntityMeleeAttack(builder, client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityMeleeAttack,
            attack_replication.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    auto fist = std::dynamic_pointer_cast<Fist>(client->player->inventory.get_main_weapon());
    if (!fist) {
        spdlog::info("[Zone] No Fist!!!");
        return;
    }

    // auto collisions = _subworld->get_collisions(fist->attack_shape.get(), static_cast<uint32_t>(ColliderLayer::ENTITIES));
    //
    // for (auto& c : collisions) {
    //     auto collider_root = c->get_root();
    //     if (!collider_root) continue;
    //
    //         auto entity = std::dynamic_pointer_cast<Entity>(collider_root);
    //     if (!entity || entity->entity_id == client->player->entity_id) continue;
    //
    //     //TODO: Calculate armor
    //     const int amount_damaged = fist->damage;
    //     entity->resource.change_health(EntityResourceChangeMode::ADD, -amount_damaged);
    //
    //     // Broadcast that entity is damaged
    //     flatbuffers::FlatBufferBuilder builder {128};
    //     const auto modify = CreateEntityResourceChange(builder,
    //         EntityResourceChangeMode::ADD, EntityResourceType::HEALTH, entity->entity_id, amount_damaged);
    //     builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityResourceChange, modify.Union()));
    //     broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    // }
}
}
