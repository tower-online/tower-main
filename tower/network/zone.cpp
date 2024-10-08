#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>
#include <tower/network/zone.hpp>
#include <tower/world/data/zone_data.hpp>

#include <tower/skill/melee_attack.hpp>

#include <cmath>
#include <fstream>
#include <filesystem>

namespace tower::network {
using namespace tower::player;

Zone::Zone(const uint32_t zone_id, boost::asio::any_io_executor& executor)
    : zone_id {zone_id}, _strand {make_strand(executor)} {}

Zone::~Zone() {
    stop();
}

std::unique_ptr<Zone> Zone::create(uint32_t zone_id, boost::asio::any_io_executor& executor,
    std::string_view zone_data_file) {
    auto zone {std::make_unique<Zone>(zone_id, executor)};

    // Read Zone data
    std::vector<uint8_t> buffer;
    {
        std::error_code ec;
        std::ifstream f {zone_data_file.data(), std::ios_base::binary};
        const auto length {std::filesystem::file_size(zone_data_file, ec)};

        if (ec) {
            spdlog::error("Invalid zone data file: {}", zone_data_file);
            return {};
        }
        buffer.resize(length);
        f.read(reinterpret_cast<char*>(buffer.data()), length);
    }

    const auto zone_data {world::data::GetZoneData(buffer.data())};
    if (flatbuffers::Verifier verifier {buffer.data(), buffer.size()}; !zone_data || !zone_data->
        Verify(verifier)) {
        spdlog::error("Invalid zone data file: {}", zone_data_file);
        return {};
    }

    const auto grid_vector {zone_data->grid()};
    const auto size_x {zone_data->size_x()}, size_z {zone_data->size_z()};
    if (size_x * size_z != grid_vector->size()) {
        spdlog::error("Invalid zone data file: {}", zone_data_file);
        return {};
    }

    Grid<bool> obstacles_grid {size_z, size_x};
    for (size_t i {0}; i < grid_vector->size(); ++i) {
        obstacles_grid.at(i) = grid_vector->Get(i)->is_blocked();
    }

    zone->_subworld = std::make_unique<Subworld>(std::move(obstacles_grid));

    return zone;
}

void Zone::handle_packet_deferred(std::shared_ptr<Packet>&& packet) {
    post(_strand, [this, packet {std::move(packet)}] mutable {
        handle_packet(std::move(packet));
    });
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
        if (_clients.empty()) start();

        _clients[client->client_id] = client;
        spdlog::debug("[Zone] ({}) Added Client({})", zone_id, client->client_id);

        const auto& player = client->player;
        player->position = glm::vec3 {_subworld->size_x() / 2, 0, _subworld->size_z() / 2};

        _subworld->add_entity(player);

        // Spawn every player in the zone
        {
            flatbuffers::FlatBufferBuilder builder {8192};
            std::vector<flatbuffers::Offset<PlayerSpawn>> spawns {};

            for (const auto& [_, client] : _clients) {
                const auto& other_player = client->player;
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
            broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->client_id);
        }
    });
}

void Zone::remove_client_deferred(const std::shared_ptr<Client>& client) {
    post(_strand, [this, client] {
        _subworld->remove_entity(client->player);
        _clients.erase(client->client_id);
        spdlog::debug("[Zone] ({}) Removed Client({})", zone_id, client->client_id);

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
    if (!_is_running) return;
    if (steady_clock::now() < _last_tick + TICK_INTERVAL) {
        post(_strand, [this] { tick(); });
        return;
    }

    _subworld->tick();
    for (auto& [_, entity] : _subworld->entities()) {
        entity->tick(this);
    }

    // Broadcast entities' transform
    {
        std::vector<EntityMovement> movements {};
        for (const auto& [_, entity] : _subworld->get_entities()) {
            movements.emplace_back(
                entity->entity_id,
                Vector3 {entity->position.x, entity->position.y, entity->position.z},
                Vector3 {entity->target_direction.x, entity->target_direction.y, entity->target_direction.z}
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
    for (const auto& [id, client] : _clients) {
        if (id == except) continue;
        client->send_packet(buffer);
    }
}

void Zone::handle_packet(std::shared_ptr<Packet>&& packet) {
    if (!_clients.contains(packet->client->client_id)) return;

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
    const auto dir_x {movement->x()}, dir_z {movement->z()};
    if (std::isnan(dir_x) || std::isnan(dir_z)) {
        return;
    }

    const auto& player = client->player;
    glm::vec3 target_direction {dir_x, 0, dir_z};

    if (dir_x != 0 && dir_z != 0) {
        target_direction = normalize(target_direction);
        player->pivot->rotation = glm::atan(target_direction.x, target_direction.z);
    }
    player->target_direction = target_direction;
}

void Zone::handle_skill_melee_attack(std::shared_ptr<Client>&& client, const SkillMeleeAttack* attack) {
    //TODO: Get weapon upon attack.weapon_slot
    // const auto weapon_slot {attack->weapon_slot()};

    auto& player {client->player};
    const auto& weapon = player->inventory.get_main_weapon();
    if (!weapon) return;
    
    const auto melee_attackable {dynamic_cast<const MeleeAttackable*>(weapon.get())};
    if (!melee_attackable) return;

    skill::MeleeAttack::use(this, player.get(), melee_attackable);
}
}
