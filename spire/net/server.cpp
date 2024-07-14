#include <spire/net/server.hpp>

#include <chrono>
#include <random>
#include <utility>

using namespace std::chrono_literals;

namespace spire::net {
Server::Server() {}

Server::~Server() {
    if (_worker_thread.joinable()) _worker_thread.join();
}

void Server::start() {
    if (_is_running.exchange(true)) return;

    listener_.start();

    // Start handling messages
    _worker_thread = std::thread([this] {
        spdlog::info("[Server] Worker thread entering");
        while (_is_running) {
            Packet packet;
            if (!_receive_queue.try_pop(packet)) {
                continue;
            }

            handle_packet(std::move(packet));
        }
        spdlog::info("[Server] Worker thread exiting");
    });
    _worker_thread.detach();

    _ctx.run();
}

void Server::stop() {
    if (!_is_running.exchange(false)) return;
}

void Server::add_client(tcp::socket&& socket) {
    static std::atomic<uint32_t> id_generator {0};

    const auto new_client_id = ++id_generator;
    auto client = std::make_shared<Client>(_ctx, std::move(socket), new_client_id, _receive_queue);
    client->start();
    spdlog::info("[Server] Added client ({})", new_client_id);

    using namespace net::packet;
    flatbuffers::FlatBufferBuilder builder {64};
    const auto client_join = CreateClientJoin(builder, client->id);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoin, client_join.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

    _clients.insert_or_assign(new_client_id, std::move(client));
}

void Server::handle_packet(Packet&& packet) {
    using namespace net::packet;

    // spdlog::debug("[Server] Handling packet from {}", client->id);

    const auto packet_base = GetPacketBase(packet.buffer.data());
    if (!packet_base) {
        spdlog::warn("[Server] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::ClientJoin:
        handle_packet_client_join(std::move(packet.client), packet_base->packet_base_as<ClientJoin>());
        break;

    default:
        break;
    }
}

void Server::handle_packet_client_join(std::shared_ptr<Client> client, const packet::ClientJoin* client_join) {
    static std::random_device rd;
    static std::mt19937 rng {rd()};
    static std::uniform_real_distribution<float> distribution {-10.0, 10.0};

    //TODO: Check if already joined

    spdlog::info("[Server] Client({}) responsed to ClientJoin", client->id);
    client->player = std::make_shared<game::Player>();
    client->player->position = glm::vec2 {distribution(rng), distribution(rng)};

    using namespace packet;
    flatbuffers::FlatBufferBuilder builder {128};
    const auto spawn_player = CreateSpawnPlayer(builder, client->id, client->player->entity_id);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::SpawnPlayer, spawn_player.Union()));

    _clients.apply_all([](std::shared_ptr<Client>& client, std::shared_ptr<flatbuffers::DetachedBuffer> buffer) {
        client->send_packet(std::move(buffer));
    }, std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}
}
