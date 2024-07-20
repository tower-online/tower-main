#include <spire/net/server.hpp>

#include <utility>

namespace spire::net {
Server::~Server() {
    stop();

    if (_worker_thread.joinable()) _worker_thread.join();
}

void Server::start() {
    if (_is_running.exchange(true)) return;

    _listener.start();
    _temp_zone->start();

    _worker_thread = std::thread([this] {
        spdlog::info("[Server] Worker thread entering");
        while (_is_running) {
            std::function<void()> job;
            if (!_jobs.try_pop(job)) continue;
            job();
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

    const auto new_id = ++id_generator;
    auto client = std::make_shared<Client>(_ctx, std::move(socket), new_id,
        [this](std::shared_ptr<Client> client) {
            spdlog::info("[Server] Client({}) disconnected", client->id);
            _clients.erase(client->id);
            if (_client_current_zones.contains(client->id)) {
                const auto current_zone = _client_current_zones.at(client->id).lock();
                current_zone->remove_client(client->id);
            }
        },
        [this](std::shared_ptr<Packet> packet) {
            _jobs.push([this, packet = std::move(packet)] {
                handle_packet(packet);
            });
        }
    );

    _jobs.push([this, client = std::move(client)] {
        client->start();
        spdlog::info("[Server] Added client ({})", client->id);

        using namespace net::packet;
        flatbuffers::FlatBufferBuilder builder {64};
        const auto client_join = CreateClientJoin(builder, client->id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoin, client_join.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        _clients.insert_or_assign(client->id, client);
    });
}

void Server::handle_packet(std::shared_ptr<Packet> packet) {
    using namespace net::packet;

    const auto packet_base = GetPacketBase(packet->buffer.data());
    if (!packet_base) {
        spdlog::warn("[Server] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::ClientJoin:
        handle_client_join(std::move(packet->client), packet_base->packet_base_as<ClientJoin>());
        break;

    default:
        break;
    }
}

void Server::handle_client_join(std::shared_ptr<Client> client, const packet::ClientJoin* client_join) {
    //TODO: Check if already joined

    if (client->id != client_join->client_id()) {
        spdlog::warn("[Server] Client({}) responsed to ClientJoin with invalid id({})",
            client->id, client_join->client_id());
        return;
    }

    //TODO: Find player's last stayed zone
    _client_current_zones.insert_or_assign(client->id, _temp_zone);
    _temp_zone->add_client(std::move(client));
}
}
