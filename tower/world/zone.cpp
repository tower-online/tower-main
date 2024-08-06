#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <tower/item/equipment/fist.hpp>
#include <tower/world/zone.hpp>
#include <tower/world/entity/mob/piggy.hpp>

#include <cmath>

namespace tower::world {
using namespace tower::player;

Zone::Zone(const uint32_t zone_id, const std::string_view tile_map_name)
    : zone_id {zone_id}, zone_name {tile_map_name},
    _tile_map {TileMap::load_tile_map(tile_map_name)} {
    _on_client_disconnected = std::make_shared<EventListener<std::shared_ptr<net::Client>>>(
        [this](std::shared_ptr<net::Client>&& client) {
            remove_client_deferred(std::move(client));
        });
}

Zone::~Zone() {
    stop();

    if (_worker_thread.joinable()) _worker_thread.join();
}

void Zone::handle_packet_deferred(std::shared_ptr<net::Packet>&& packet) {
    // std::function cannot capture move-only type; e.g) std::unique_ptr
    _jobs.push([this, packet {std::move(packet)}]() mutable {
        handle_packet(std::move(packet));
    });
}

void Zone::start() {
    if (_is_running.exchange(true)) return;

    _worker_thread = std::thread([this] {
        auto last_tick_time = steady_clock::now();

        while (_is_running) {
            std::queue<std::function<void()>> jobs;
            _jobs.swap(jobs);
            while (!jobs.empty()) {
                const auto& job = jobs.front();
                job();
                jobs.pop();
            }

            const auto current_time = steady_clock::now();
            if (current_time - last_tick_time < TICK_INTERVAL) continue;
            tick();
            last_tick_time = current_time;
        }
    });
    _worker_thread.detach();

    const auto piggy = game::Piggy::create();
    _entities[piggy->entity_id] = piggy;
    piggy->position = {_tile_map.get_size().x * Tile::TILE_SIZE / 2 + 100, _tile_map.get_size().y * Tile::TILE_SIZE / 2};
    add_collision_objects_from_tree(piggy);
}

void Zone::stop() {
    if (!_is_running.exchange(false)) return;

    //TODO: Flush jobs before stopping
}

void Zone::add_client_deferred(std::shared_ptr<net::Client>&& client) {
    if (_clients.empty()) {
        start();
    }

    _jobs.push([this, client = std::move(client)] {
        _clients[client->id] = client;
        client->disconnected.subscribe(_on_client_disconnected->shared_from_this());

        const auto& player = client->player;

        _entities[player->entity_id] = player;
        player->position = {_tile_map.get_size().x * Tile::TILE_SIZE / 2, _tile_map.get_size().y * Tile::TILE_SIZE / 2};
        add_collision_objects_from_tree(player);

        flatbuffers::FlatBufferBuilder builder {128};
        const auto enter = CreatePlayerEnterZoneDirect(builder, zone_id, zone_name.c_str());
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerEnterZone, enter.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    });
}

void Zone::remove_client_deferred(std::shared_ptr<net::Client>&& client) {
    _jobs.push([this, client] {
        _clients.erase(client->id);
        remove_collision_objects_from_tree(client->player);
        _entities.erase(client->player->entity_id);
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

void Zone::tick() {
    if (_entities.empty()) return;

    // Move entities
    std::vector<EntityMovement> movements {};
    for (auto& [_, entity] : _entities) {
        // Check if tile is blocked and move
        //TODO: Pull out if entity is already in collider
        if (entity->target_direction != glm::vec2 {0.0f, 0.0f}) {
            const auto target_position = entity->position + entity->target_direction * entity->movement_speed_base;
            if (Tile tile; _tile_map.try_at(target_position, tile) && tile.state != TileState::BLOCKED) {
                entity->position = target_position;
            }
        }

        movements.push_back(EntityMovement {
            entity->entity_id,
            net::packet::Vector2 {entity->position.x, entity->position.y},
            net::packet::Vector2 {entity->target_direction.x, entity->target_direction.y}
        });
    }

    // Broadcast entities' transform
    flatbuffers::FlatBufferBuilder builder {};
    const auto entity_movements = CreateEntityMovementsDirect(builder, &movements);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityMovements, entity_movements.Union()));
    broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

    // Update contacts
    for (const auto& [_, area] : _collision_areas) {
        for (const auto& [_, body] : _collision_objects) {
            if (area->is_colliding(body)) {
                if (_contacts[area->node_id].contains(body->node_id)) {
                    area->body_staying.notify(body);
                } else {
                    area->body_entered.notify(body);
                    _contacts[area->node_id].insert(body->node_id);
                }
            } else if (_contacts[area->node_id].contains(body->node_id)) {
                area->body_exited.notify(body);
                _contacts[area->node_id].erase(body->node_id);
            }
        }
    }
}

void Zone::broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer, const uint32_t except) {
    for (auto& [id, client] : _clients) {
        if (id == except) continue;
        client->send_packet(buffer);
    }
}

void Zone::handle_packet(std::shared_ptr<net::Packet>&& packet) {
    if (!_clients.contains(packet->client->id)) return;

    const auto packet_base = GetPacketBase(packet->buffer.data());
    if (!packet_base) {
        spdlog::warn("[Zone] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::PlayerMovement:
        hanlde_player_movement(std::move(packet->client), packet_base->packet_base_as<PlayerMovement>());
        break;

    case PacketType::PlayerEnterZone:
        handle_player_enter_zone(std::move(packet->client), packet_base->packet_base_as<PlayerEnterZone>());
        break;

    case PacketType::EntityMeleeAttack:
        hanlde_entity_melee_attack(std::move(packet->client), packet_base->packet_base_as<EntityMeleeAttack>());
        break;

    default:
        break;
    }
}

void Zone::hanlde_player_movement(std::shared_ptr<net::Client>&& client, const PlayerMovement* movement) {
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

void Zone::handle_player_enter_zone(std::shared_ptr<net::Client>&& client, const PlayerEnterZone* enter) {
    // Spawn every entities in the zone
    for (const auto& [_, entity] : _entities) {
        flatbuffers::FlatBufferBuilder builder {256};
        const Vector2 position {entity->position.x, entity->position.y};


        const auto spawn = CreateEntitySpawn(builder,
            entity->entity_type, entity->entity_id, &position, entity->rotation);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawn, spawn.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    // Notify other clients for new player spawn
    {
        const auto& player = client->player;

        flatbuffers::FlatBufferBuilder builder {256};
        const Vector2 position {player->position.x, player->position.y};
        const auto spawn = CreateEntitySpawn(builder,
            player->entity_type, player->entity_id, &position, player->rotation);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawn, spawn.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->id);
    }
}

void Zone::hanlde_entity_melee_attack(std::shared_ptr<net::Client>&& client, const EntityMeleeAttack* attack) {
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

    auto collisions = get_collisions(fist->attack_shape.get(), static_cast<uint32_t>(ColliderLayer::ENTITIES));

    for (auto& c : collisions) {
        auto collider_root = c->get_root();
        if (!collider_root) continue;

        auto entity = std::dynamic_pointer_cast<Entity>(collider_root);
        if (!entity || entity->entity_id == client->player->entity_id) continue;

        //TODO: Calculate armor
        const auto amount_damaged = fist->damage;
        entity->modify_resource(EntityResourceType::HEALTH, EntityResourceModifyMode::NEGATIVE, amount_damaged);

        // Brodacst that entity is damaged
        flatbuffers::FlatBufferBuilder builder {128};
        const auto modify = CreateEntityResourceModify(builder,
            EntityResourceModifyMode::NEGATIVE, EntityResourceType::HEALTH, entity->entity_id,
            amount_damaged, entity->resource.health);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityResourceModify, modify.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }
}

void Zone::add_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            _collision_objects[object->node_id] = object;
        }
        add_collision_objects_from_tree(child);
    }
}

void Zone::remove_collision_objects_from_tree(const std::shared_ptr<Node>& node) {
    for (auto& child : node->get_childs()) {
        if (const auto object = std::dynamic_pointer_cast<CollisionObject>(child)) {
            _collision_objects.erase(object->node_id);
        }
        remove_collision_objects_from_tree(child);
    }
}

void Zone::add_collision_area(const std::shared_ptr<CollisionObject>& area) {
    _collision_areas[area->node_id] = area;
    _contacts[area->node_id] = {};
}

void Zone::remove_collision_area(const uint32_t area_id) {
    _collision_areas.erase(area_id);
    _contacts.erase(area_id);
}

std::vector<std::shared_ptr<CollisionObject>> Zone::get_collisions(const std::shared_ptr<CollisionObject>& collider) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, other] : _collision_objects) {
        if (collider->is_colliding(other)) continue;

        collisions.push_back(other);
    }

    return collisions;
}

std::vector<std::shared_ptr<CollisionObject>> Zone::get_collisions(const CollisionShape* target_shape,
    const uint32_t mask) {
    std::vector<std::shared_ptr<CollisionObject>> collisions {};
    for (auto& [_, c] : _collision_objects) {
        if (!(mask & c->layer)) continue;

        if (!c->shape->is_colliding(target_shape)) continue;
        collisions.push_back(c);
    }

    return collisions;
}
}
