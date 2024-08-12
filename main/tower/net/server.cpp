#include <tower/net/server.hpp>

namespace tower::net {
Server::Server() {
    _on_client_disconnected = std::make_shared<EventListener<std::shared_ptr<Client>>>(
        [this](std::shared_ptr<Client>&& client) {
            remove_client(std::move(client));
        }
    );

    // default zone
    _zones.insert_or_assign(0, std::make_unique<Zone>(0, "test_zone"));
}

Server::~Server() {
    stop();

    if (_worker_thread.joinable()) _worker_thread.join();
}

void Server::start() {
    if (_is_running.exchange(true)) return;

    _listener.start();

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
    auto new_client = std::make_shared<Client>(_ctx, std::move(socket), new_id,
        [this](std::shared_ptr<Client>&& client, std::vector<uint8_t>&& buffer) {
            handle_packet(std::make_unique<Packet>(std::move(client), std::move(buffer)));
        }
    );

    _jobs.push([this, client = std::move(new_client)] {
        _clients.insert_or_assign(client->id, client);
        client->start();
        client->disconnected.subscribe(_on_client_disconnected->shared_from_this());

        spdlog::info("[Server] Added client ({})", client->id);

        using namespace net::packet;
        flatbuffers::FlatBufferBuilder builder {128};
        const auto client_join = CreateClientJoin(builder, client->id, client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoin, client_join.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    });
}

void Server::remove_client(std::shared_ptr<Client>&& client) {
    _jobs.push([this, client = std::move(client)] {
        _clients.erase(client->id);
        _clients_current_zone.erase(client->id);
        spdlog::info("[Server] Removed client ({})", client->id);
    });
}

void Server::handle_packet(std::unique_ptr<Packet> packet) {
    using namespace net::packet;

    const auto packet_base = GetPacketBase(packet->buffer.data());
    if (!packet_base) {
        spdlog::warn("[Server] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet_base->packet_base_type()) {
    case PacketType::ClientJoin:
        handle_client_join_deferred(std::move(packet->client), packet_base->packet_base_as<ClientJoin>());
        break;

    default:
        _zones[_clients_current_zone[packet->client->id]]->handle_packet_deferred(std::move(packet));
        break;
    }
}

void Server::handle_client_join_deferred(std::shared_ptr<Client>&& client, const ClientJoin* client_join) {
    if (client->id != client_join->client_id()) {
        spdlog::warn("[Server] Client({}) responsed to ClientJoin with invalid id({})",
            client->id, client_join->client_id());
        return;
    }

    _jobs.push([this, client = std::move(client)]() mutable {
        //TODO: Find player's last stayed zone
        _clients_current_zone.insert_or_assign(client->id, 0);
        _zones[0]->add_client_deferred(std::move(client));
    });
}
}