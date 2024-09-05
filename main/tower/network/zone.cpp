#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <tower/item/equipment/fist.hpp>
#include <tower/network/zone.hpp>
#include <tower/system/math.hpp>
#include <tower/entity/entity.hpp>

#include <cmath>

namespace tower::network {
using namespace tower::player;

Zone::Zone(const uint32_t zone_id, boost::asio::thread_pool& work_ctx)
    : zone_id {zone_id}, _work_ctx {work_ctx} {}

Zone::~Zone() {
    stop();
}

void Zone::handle_packet_deferred(std::shared_ptr<Packet>&& packet) {
    // std::function cannot capture move-only type; e.g) std::unique_ptr
    _jobs.push([this, packet {std::move(packet)}]() mutable {
        handle_packet(std::move(packet));
    });
}

void Zone::init(std::string_view tile_map) {
    _subworld = std::make_unique<Subworld>(tile_map);
}

void Zone::start() {
    if (_is_running.exchange(true)) return;

    post(_work_ctx, [this] {
        handle_jobs(true);
    });
}

void Zone::stop() {
    if (!_is_running.exchange(false)) return;
}

void Zone::add_client_deferred(std::shared_ptr<Client>&& client) {
    // Start in case of clients are empty so that zone has been stopped.
    start();

    _jobs.push([this, client = std::move(client)] {
        _clients[client->id] = client;
        _clients_on_disconnected[client->id] = client->disconnected.connect(
            [this](std::shared_ptr<Client> disconnecting_client) {
                remove_client_deferred(std::move(disconnecting_client));
            });

        const auto& player = client->player;
        // player->position = {_subworld.get_size().x * Tile::TILE_SIZE / 2, _subworld.get_size().y * Tile::TILE_SIZE / 2};
        player->position = {0, 0};

        _subworld->add_entity(player);

        {
            flatbuffers::FlatBufferBuilder builder {128};
           const auto enter = CreatePlayerEnterZoneDirect(builder, zone_id, _subworld->get_tilemap().get_name().data());
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
            const auto& player = client->player;

            std::vector<EntitySpawn> spawns {};
            spawns.emplace_back(player->entity_type, player->entity_id,
                Vector2 {player->position.x, player->position.y}, player->rotation);

            flatbuffers::FlatBufferBuilder builder {256};
            const auto spawn = CreateEntitySpawnsDirect(builder, &spawns);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, spawn.Union()));
            broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->id);
        }
    });
}

void Zone::remove_client_deferred(std::shared_ptr<Client>&& client) {
    _jobs.push([this, client] {
        _clients.erase(client->id);
        _clients_on_disconnected.erase(client->id);
        _subworld->remove_entity(client->player);
        spdlog::info("[Zone] Removed client ({})", client->id);

        // Broadcast EntityDespawn
        flatbuffers::FlatBufferBuilder builder {64};
        const auto entity_despawn = CreateEntityDespawn(builder, client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityDespawn, entity_despawn.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        if (_clients.empty()) {
            stop();
        }
    });
}

void Zone::handle_jobs(const bool loop) {
    std::queue<std::function<void()>> jobs;
    _jobs.swap(jobs);

    while (!jobs.empty()) {
        const auto& job = jobs.front();
        job();
        jobs.pop();
    }

    if (const auto now = steady_clock::now(); now >= _last_tick + TICK_INTERVAL) {
        tick();
        _last_tick = now;
    }

    if (!loop || !_is_running) return;
    post(_work_ctx, [this] {
        handle_jobs(true);
    });
}

void Zone::tick() {
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
}

void Zone::broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, const uint32_t except) {
    for (auto& [id, client] : _clients) {
        if (id == except) continue;
        client->send_packet(buffer);
    }
}

void Zone::handle_packet(std::shared_ptr<Packet>&& packet) {
    if (!_clients.contains(packet->client->id)) return;

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
