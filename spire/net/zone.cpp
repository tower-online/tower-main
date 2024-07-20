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

    _jobs.push([this, client = std::move(client)] {
        _clients.insert_or_assign(client->id, client);

        using namespace packet;
        // Spawn player on the random position
        {
            static std::random_device rd;
            static std::mt19937 rng {rd()};
            static std::uniform_real_distribution<float> distribution {-30.0, 30.0};

            client->player = std::make_shared<game::Player>();
            client->player->position = glm::vec2 {distribution(rng), distribution(rng)};

            flatbuffers::FlatBufferBuilder builder {128};
            const Vector2 position {client->player->position.x, client->player->position.y};
            const auto spawn_player = CreateSpawnPlayer(builder, client->id, client->player->entity_id, &position);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::SpawnPlayer, spawn_player.Union()));

            broadcast_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Spawn other players already in the zone
        for (const auto& [_, other_client] : _clients) {
            if (other_client == client) continue;

            flatbuffers::FlatBufferBuilder builder {128};
            const Vector2 position {other_client->player->position.x, other_client->player->position.y};
            const auto spawn_player = CreateSpawnPlayer(builder, other_client->id, other_client->player->entity_id, &position);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::SpawnPlayer, spawn_player.Union()));

            client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }
    });
}

void Zone::remove_client(const uint32_t client_id) {
    _jobs.push([this, client_id] {
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
    case PacketType::EntityTransformUpdate:
        hanlde_entity_transform_update(std::move(packet->client), packet_base->packet_base_as<EntityTransformUpdate>());
        break;

    default:
        break;
    }
}

void Zone::hanlde_entity_transform_update(std::shared_ptr<Client>&& client, const packet::EntityTransformUpdate* update) {
    const auto position = (*update->entity_transforms())[0]->position();
    client->player->position = glm::vec2 {position.x(), position.y()};
}

void Zone::tick() {
    // Update entities' transform
    using namespace packet;
    std::vector<EntityTransform> entity_transforms {};
    for (auto& [_, client] : _clients) {
        const auto& position = client->player->position;
        entity_transforms.emplace_back(
            client->player->entity_id,
            Vector2 {position.x, position.y});
    }

    flatbuffers::FlatBufferBuilder builder;
    const auto entity_transform_update = CreateEntityTransformUpdateDirect(builder, &entity_transforms);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityTransformUpdate,
        entity_transform_update.Union()));
    broadcast_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}
}
