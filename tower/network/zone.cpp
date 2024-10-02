#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <tower/entity/entity.hpp>
#include <tower/network/zone.hpp>
#include <tower/player/skill/melee_attack.hpp>

#include <cmath>

namespace tower::network {
using namespace tower::player;

static constexpr glm::vec2 zero2 {0, 0};
static constexpr glm::vec3 zero3 {0, 0, 0};

Zone::Zone(const uint32_t zone_id, boost::asio::any_io_executor& executor)
    : zone_id {zone_id}, _strand {make_strand(executor)} {}

Zone::~Zone() {
    stop();
}

void Zone::handle_packet_deferred(std::shared_ptr<Packet>&& packet) {
    post(_strand, [this, packet {std::move(packet)}] mutable {
        handle_packet(std::move(packet));
    });
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

void Zone::add_client_deferred(const std::shared_ptr<Client>& client) {
    post(_strand, [this, client] {
        if (_client_entries.empty()) start();

        auto client_entry = std::make_unique<ClientEntry>(
            client,
            client->disconnected.connect([this](const std::shared_ptr<Client>& c) {
                remove_client_deferred(c);
            }));
        _client_entries.insert_or_assign(client_entry->client->entry_id, std::move(client_entry));

        const auto& player = client->player;
        player->position = zero3;

        _subworld->add_entity(player);

        // Spawn every player in the zone
        {
            flatbuffers::FlatBufferBuilder builder {8192};
            std::vector<flatbuffers::Offset<PlayerSpawn>> spawns {};

            for (const auto& [_, entry] : _client_entries) {
                const auto& other_player = entry->client->player;
                if (other_player->entity_id == player->entity_id) continue;

                spawns.emplace_back(CreatePlayerSpawn(builder,
                    false, other_player->entity_id, other_player->write_player_info(builder)));
            }
            const auto spawns_offset = builder.CreateVector(spawns);

            const auto player_spawns = CreatePlayerSpawns(builder, spawns_offset);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawns, player_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Spawn every entity in the zone
        {
            std::vector<EntitySpawn> spawns {};
            for (const auto& [entity_id, entity] : _subworld->get_entities()) {
                // Skip Player type
                if (dynamic_cast<Player*>(entity.get()) != nullptr) continue;

                spawns.emplace_back(entity->entity_type, entity->entity_id,
                    Vector3 {entity->position.x, entity->position.y, entity->position.z}, entity->rotation);
            }

            flatbuffers::FlatBufferBuilder builder {1024};
            const auto entity_spawns = CreateEntitySpawnsDirect(builder, &spawns);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, entity_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Notify other clients for new player spawn
        {
            flatbuffers::FlatBufferBuilder builder {1024};
            const auto spawn = CreatePlayerSpawn(builder, false, player->entity_id, player->write_player_info(builder));
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawn, spawn.Union()));
            broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->entry_id);
        }
    });
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
    if (!_is_running) return;
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
                Vector3 {entity->position.x, entity->position.y, entity->position.z},
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

    case PacketType::SkillMeleeAttack:
        handle_skill_melee_attack(std::move(packet->client), packet_base->packet_base_as<SkillMeleeAttack>());
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
    if (target_direction != zero2) {
        target_direction = normalize(target_direction);
    }

    const auto& player = client->player;
    player->target_direction = target_direction;
    if (player->target_direction != zero2) {
        player->pivot->rotation = glm::atan(target_direction.y, target_direction.x);
    }
}

void Zone::handle_skill_melee_attack(std::shared_ptr<Client>&& client, const SkillMeleeAttack* attack) {
    //TODO: Get weapon upon attack.weapon_slot
    const auto weapon_slot {attack->weapon_slot()};

    auto& player {client->player};
    const auto& weapon = player->inventory.get_main_weapon();
    if (!weapon) return;
    
    const auto melee_attackable {dynamic_cast<const MeleeAttackable*>(weapon.get())};
    if (!melee_attackable) return;
    if (!player->state_machine.try_transition("Attacking")) return;

    const auto colliders =  _subworld->get_collisions(
        melee_attackable->melee_attack_shape(), std::to_underlying(ColliderLayer::ENTITIES));

    std::vector<EntityResourceChange> damages {};
    for (const auto& collider : colliders) {
        Entity* entity;
        if (entity = dynamic_cast<Entity*>(collider->get_root().get()); !entity) continue;
        if (player->entity_id == entity->entity_id) continue; // Don't attack myself

        //TODO: Caculate damage
        const int damage {melee_attackable->melee_attack_damage()};
        damages.emplace_back(EntityResourceChangeMode::ADD, EntityResourceType::HEALTH, entity->entity_id, damage);

    }
    // Broadcast that entity is damaged
    {
        flatbuffers::FlatBufferBuilder builder {512};
        const auto changes = CreateEntityResourceChangesDirect(builder, &damages);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityResourceChanges, changes.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }
    // Brodacast melee attack
    {
        flatbuffers::FlatBufferBuilder builder {128};
        const auto attack_replication = CreateSkillMeleeAttack(builder, client->player->entity_id, weapon_slot);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::SkillMeleeAttack, attack_replication.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    co_spawn(_strand, [this, player]->boost::asio::awaitable<void> {
        //TODO: Get attack period from weapon
        boost::asio::steady_timer timer {_strand, 1000ms};
        auto [_] = co_await timer.async_wait(as_tuple(boost::asio::use_awaitable));
        //TODO: Check if player is still in the zone?
        player->state_machine.try_transition("Idle");
    }, boost::asio::detached);
}
}
