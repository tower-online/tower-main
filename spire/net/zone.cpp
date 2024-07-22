#include <spire/net/zone.hpp>

#include <random>

namespace spire::net {
Zone::~Zone() {
    stop();

    if (_worker_thread.joinable()) _worker_thread.join();
}

void Zone::start() {
    if (_is_running.exchange(true)) return;

    _worker_thread = std::thread([this] {
        auto last_tick_time = steady_clock::now();

        while (_is_running) {
            for (size_t i {_jobs.size()}; i > 0; --i) {
                std::function<void()> job;
                if (!_jobs.try_pop(job)) break;
                job();
            }

            const auto current_time = steady_clock::now();
            if (current_time - last_tick_time < TICK_INTERVAL) continue;
            tick();
            last_tick_time = current_time;
        }
    });
    _worker_thread.detach();
}

void Zone::stop() {
    if (!_is_running.exchange(false)) return;
}

void Zone::add_client(std::shared_ptr<Client> client) {
    client->set_packet_received([this](std::shared_ptr<Packet> packet) {
        _jobs.push([this, packet = std::move(packet)]() mutable {
            handle_packet(std::move(packet));
        });
    });

    _jobs.push([this, new_client = std::move(client)] {
        _clients.insert_or_assign(new_client->id, new_client);

        // Set player on the random position
        static std::random_device rd;
        static std::mt19937 rng {rd()};
        static std::uniform_real_distribution<float> distribution {-30.0, 30.0};

        new_client->player->position = glm::vec2 {distribution(rng), distribution(rng)};

        // Spawn players in the zone
        using namespace packet;
        for (const auto& [_, client] : _clients) {
            flatbuffers::FlatBufferBuilder builder {256};
            const Vector2 position {client->player->position.x, client->player->position.y};
            const Vector2 direction {client->player->direction.x, client->player->direction.y};
            const auto spawn = CreateEntitySpawn(builder,
                EntityType::PLAYER, client->player->entity_id, &position, &direction);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawn, spawn.Union()));
            new_client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Notify new player spawn to other players
        {
            flatbuffers::FlatBufferBuilder builder {256};
            const Vector2 position {new_client->player->position.x, new_client->player->position.y};
            const Vector2 direction {new_client->player->direction.x, new_client->player->direction.y};
            const auto spawn = CreateEntitySpawn(builder,
                EntityType::PLAYER, new_client->player->entity_id, &position, &direction);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntitySpawn, spawn.Union()));
            broadcast_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()), new_client->id);
        }
    });
}

void Zone::remove_client(const uint32_t client_id) {
    _jobs.push([this, client_id] {
        if (!_clients.contains(client_id)) return;

        const auto client = _clients[client_id];

        // Broadcast EntityDespawn packet
        using namespace packet;
        flatbuffers::FlatBufferBuilder builder {64};
        const auto entity_despawn = CreateEntityDespawn(builder, client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityDespawn, entity_despawn.Union()));
        broadcast_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        _clients.erase(client_id);
    });
}

void Zone::broadcast_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer, const uint32_t except) {
    for (auto& [id, client] : _clients) {
        if (id == except) continue;
        client->send_packet(buffer);
    }
}

void Zone::handle_packet(std::shared_ptr<Packet>&& packet) {
    using namespace net::packet;

    const auto packet_base = GetPacketBase(packet->buffer.data());
    if (!packet_base) {
        spdlog::warn("[Server] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::EntityMovement:
        hanlde_entity_movement(std::move(packet->client), packet_base->packet_base_as<EntityMovement>());
        break;

    default:
        break;
    }
}

void Zone::hanlde_entity_movement(std::shared_ptr<Client>&& client, const packet::EntityMovement* movement) {
    const auto direction = movement->direction();
    client->player->direction = direction->x() == 0 && direction->y() == 0
        ? glm::vec2 {0, 0}
        : normalize(glm::vec2 {direction->x(), direction->y()});
}

void Zone::tick() {
    // Move entities
    for (auto& [_, client] : _clients) {
        auto& position = client->player->position;
        const auto& direction = client->player->direction;

        if (length(direction) == 0) continue;
        //TODO: Check collisions before move
        position += direction * game::Player::MOVEMENT_SPEED;
    }

    // Update entities' transform
    using namespace packet;
    for (auto& [_, client] : _clients) {
        Vector2 position {client->player->position.x, client->player->position.y};
        Vector2 direction {client->player->direction.x, client->player->direction.y};

        flatbuffers::FlatBufferBuilder builder {128};
        const auto entity_transform_update = CreateEntityMovement(builder,
            client->player->entity_id, &position, &direction);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityMovement,
            entity_transform_update.Union()));
        broadcast_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }
}
}
