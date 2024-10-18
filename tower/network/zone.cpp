#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <spdlog/spdlog.h>
#include <tower/item/item_factory.hpp>
#include <tower/item/item_object.hpp>
#include <tower/item/gold.hpp>
#include <tower/network/zone.hpp>
#include <tower/world/data/zone_data.hpp>
#include <tower/system/random.hpp>

#include <tower/skill/melee_attack.hpp>

#include <cmath>
#include <fstream>
#include <filesystem>


namespace tower::network {
using namespace tower::player;
using namespace tower::item;

Zone::Zone(const uint32_t zone_id, boost::asio::any_io_executor& executor, const std::shared_ptr<ServerSharedState>& shared_state)
    : zone_id {zone_id}, _strand {make_strand(executor)}, _shared_state {shared_state} {}

Zone::~Zone() {
    stop();
}

std::unique_ptr<Zone> Zone::create(uint32_t zone_id, boost::asio::any_io_executor& executor,
    std::string_view zone_data_file, const std::shared_ptr<ServerSharedState>& shared_state) {
    auto zone {std::make_unique<Zone>(zone_id, executor, shared_state)};

    // Read Zone obstacles data
    {
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
            f.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(length));
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
    }

    //TODO: Read from the file
    if (zone_id == 1) {
        auto spawner {std::make_unique<EntitySpawner>()};
        spawner->entity_type = EntityType::SIMPLE_MONSTER;
        spawner->max_entities_count = 3;
        spawner->spawn_interval = 5s;
        spawner->spawn_position = glm::vec3 {zone->subworld()->size_x() / 2, 0, zone->subworld()->size_z() / 2};

        zone->add_spawner(std::move(spawner));
    }

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
        // spdlog::debug("[Zone] ({}) Added Client({})", zone_id, client->client_id);

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
                    false, other_player->entity_id, other_player->owner_id, other_player->write_player_info(builder)));
            }
            const auto spawns_offset = builder.CreateVector(spawns);

            const auto player_spawns = CreatePlayerSpawns(builder, spawns_offset);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawns, player_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Spawn every entity in the zone
        {
            flatbuffers::FlatBufferBuilder builder {1024};

            std::vector<flatbuffers::Offset<EntitySpawn>> spawns {};
            for (const auto& [entity_id, entity] : _subworld->entities()) {
                // Skip Player type
                if (dynamic_cast<Player*>(entity.get()) != nullptr) continue;

                const Vector3 position {entity->position.x, entity->position.y, entity->position.z};
                auto spawn {CreateEntitySpawn(builder, entity->entity_type, entity_id, &position, entity->rotation,
                    entity->resource.health, entity->resource.max_health)};
                spawns.emplace_back(spawn);
            }

            const auto entity_spawns = CreateEntitySpawnsDirect(builder, &spawns);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, entity_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Spawn every item in the zone
        {
            flatbuffers::FlatBufferBuilder builder {1024};
            std::vector<flatbuffers::Offset<ItemSpawn>> spawns {};

            for (const auto& [_, object] : _subworld->objects()) {
                auto item_object {dynamic_cast<const ItemObject*>(object.get())};
                if (!item_object) continue;

                const auto spawn {CreateItemSpawn(builder,
                    item_object->object_id, item_object->item->type, Item::get_amount(item_object->item.get()))};
                spawns.push_back(spawn);
            }

            const auto item_spawns {CreateItemSpawnsDirect(builder, &spawns)};
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ItemSpawns, item_spawns.Union()));
            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Notify other clients for new player spawn
        {
            flatbuffers::FlatBufferBuilder builder {1024};
            const auto spawn = CreatePlayerSpawn(builder,
                false, player->entity_id, client->client_id, player->write_player_info(builder));
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawn, spawn.Union()));
            broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), client->client_id);
        }
    });
}

void Zone::remove_client_deferred(const std::shared_ptr<Client>& client) {
    post(_strand, [this, client] {
        if (!_clients.contains(client->client_id)) return;

        _clients.erase(client->client_id);

        const auto player_entity {std::static_pointer_cast<Entity>(client->player)};
        despawn_entity(player_entity);
        // spdlog::debug("[Zone] ({}) Removed Client({})", zone_id, client->client_id);

        if (_clients.empty()) {
            stop();
        }
    });
}

void Zone::spawn_entity_deferred(std::shared_ptr<Entity> entity) {
    post(_strand, [this, entity = std::move(entity)] mutable {
        entity->dead.connect([this](const uint32_t entity_id, std::shared_ptr<Entity> killer) {
            if (!_subworld->entities().contains(entity_id)) return;

            auto& self = _subworld->entities().at(entity_id);
            spdlog::debug("Zone({}): entity({}) dead by ({})", zone_id, self->entity_id, killer->entity_id);
            despawn_entity(self);

            // Random item drop
            //TODO: Get drop data from dead entity instead of hardcoded array
            const std::array<ItemType, 2> items {{ItemType::GOLD, ItemType::FIST}};
            const std::array<float, 2> weights {{0.5, 0.3}};
            const ItemFactory::ItemCreateConfig config {
                .type = *random_select(items.begin(), items.end(), weights.begin(), weights.end()),
                .rarity_min = ItemRarity::NORMAL,
                .rarity_max = ItemRarity::LEGENDARY,
            };

            auto new_item {ItemFactory::create(config)};
            auto new_item_object {std::make_shared<ItemObject>(std::move(new_item))};
            const auto new_item_amount {Item::get_amount(new_item_object->item.get())};

            // Broadcast item drop
            {
                flatbuffers::FlatBufferBuilder builder {128};
                Vector3 spawn_position {self->position.x, self->position.y, self->position.z};
                const auto spawn {CreateItemSpawn(builder,
                    new_item_object->object_id, new_item_object->item->type, new_item_amount, &spawn_position)};
                builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ItemSpawn, spawn.Union()));
                broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
            }

            _subworld->add_object(new_item_object);
        });


        //Send entity spawn packet
        flatbuffers::FlatBufferBuilder builder {256};

        std::vector<flatbuffers::Offset<EntitySpawn>> spawns {};
        const Vector3 position {entity->position.x, entity->position.y, entity->position.z};
        auto spawn {CreateEntitySpawn(builder, entity->entity_type, entity->entity_id, &position, entity->rotation,
            entity->resource.health, entity->resource.max_health)};
        spawns.emplace_back(spawn);

        const auto entity_spawns = CreateEntitySpawnsDirect(builder, &spawns);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawns, entity_spawns.Union()));
        broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        _subworld->add_entity(entity);
    });
}

void Zone::despawn_entity(const std::shared_ptr<Entity>& entity) {
    _subworld->remove_entity(entity);

    // Broadcast EntityDespawn
    flatbuffers::FlatBufferBuilder builder {64};
    const auto entity_despawn = CreateEntityDespawn(builder, entity->entity_id);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityDespawn, entity_despawn.Union()));
    broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Zone::add_spawner(std::unique_ptr<EntitySpawner> spawner) {
    spawner->spawned.connect([this](std::shared_ptr<Entity> entity) {
        spawn_entity_deferred(std::move(entity));
    });

    _spawners.push_back(std::move(spawner));
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

    for (const auto& spawner : _spawners) {
        spawner->tick();
    }

    // Broadcast entities' transform
    {
        std::vector<EntityMovement> movements {};
        for (const auto& [_, entity] : _subworld->entities()) {
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

    case PacketType::PlayerPickupItem:
        handle_player_pickup_item(std::move(packet->client), packet_base->packet_base_as<PlayerPickupItem>());
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
    
    if (!all(epsilonEqual(target_direction, zero3, 1e-5f))) {
        target_direction = normalize(target_direction);
        player->rotation = glm::atan(target_direction.x, target_direction.z);
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

    auto player_entity {std::static_pointer_cast<Entity>(player)};
    skill::MeleeAttack::use(this, player_entity, melee_attackable);
}

void Zone::handle_player_pickup_item(std::shared_ptr<Client>&& client, const PlayerPickupItem* pickup) {
    //TODO: Check if close enough
    spdlog::debug("Zone({}) Player({}) picking up", zone_id, client->player->character_id());

    const auto object_id {pickup->object_id()};
    if (!_subworld->has_object(object_id)) return;

    const auto& object {_subworld->get_object(object_id)};
    auto item_object {std::dynamic_pointer_cast<ItemObject>(object)};
    if (!item_object) return;

    // Move to client's inventory
    client->player->inventory.add_item(item_object->item);
    _subworld->remove_object(object);
    spdlog::debug("Player({}) golds: {}", client->player->character_id(), client->player->inventory.golds());

    // Delegate db task to server
    co_spawn(_shared_state->server_strand, [this, client, item_object] -> boost::asio::awaitable<void> {
        auto [ec, conn] {
            co_await _shared_state->db_pool.async_get_connection(as_tuple(boost::asio::use_awaitable))};
        if (ec) {
            spdlog::error("Character({}) error getting db connection", client->player->character_id());
            co_return;
        }

        bool result;
        if (const auto gold {dynamic_cast<const Gold*>(item_object->item.get())}; gold) {
            result = co_await Inventory::save_item(conn, client->player->character_id(), item_object->item);
        } else {
            result = co_await Inventory::save_gold(conn, client->player->character_id(), client->player->inventory.golds());
        }
        if (!result) {
            spdlog::warn("Character({}) error saving item", client->player->character_id());
        }
    }, boost::asio::detached);

    // Broadcast item despawn
    flatbuffers::FlatBufferBuilder builder {64};
    const auto item_despawn {CreateItemDespawn(builder, object->object_id)};
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ItemDespawn, item_despawn.Union()));
    broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}
}
