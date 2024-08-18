#include <jwt-cpp/jwt.h>
#include <tower/net/server.hpp>

namespace tower::net {
Server::Server(const size_t num_io_threads, const size_t num_worker_threads)
    : _num_io_threads {num_io_threads}, _worker_threads {num_worker_threads} {
    _on_client_disconnected = std::make_shared<EventListener<std::shared_ptr<Client>>>(
        [this](std::shared_ptr<Client>&& client) {
            remove_client_deferred(std::move(client));
        }
    );

    // default zone
    _zones.insert_or_assign(0, std::make_unique<Zone>(0, _worker_threads));
    _zones[0]->init("test_zone");

    redis::config redis_config {};
    redis_config.addr.host = Settings::redis_host();
    redis_config.password = Settings::redist_password();
    _redis_connection.async_run(redis_config, {}, boost::asio::detached);
}

Server::~Server() {
    stop();

    _io_threads.join_all();
    _worker_threads.join();
}

void Server::start() {
    if (_is_running.exchange(true)) return;
    spdlog::info("[Server] Starting...");

    _listener.start();

    _io_guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        make_work_guard(_ctx));
    for (auto i {0}; i < _num_io_threads; ++i) {
        _io_threads.create_thread([this] {
            while (_is_running) {
                _ctx.run();
            }
        });
    }

    post(_worker_threads, [this] {
        handle_jobs(true);
    });
}

void Server::stop() {
    if (!_is_running.exchange(false)) return;
    spdlog::info("[Server] Stopping...");

    _io_guard->reset();
    _io_guard = nullptr;

    for (auto& [_, client] : _clients) {
        client->stop();
    }

    for (auto& [_, zone] : _zones) {
        zone->stop();
    }

    _listener.stop();
}

void Server::join() {
    _io_threads.join_all();
    _worker_threads.join();
}

void Server::handle_jobs(const bool loop) {
    std::queue<std::function<void()>> jobs;
    _jobs.swap(jobs);

    while (!jobs.empty()) {
        const auto& job = jobs.front();
        job();
        jobs.pop();
    }

    if (!loop || !_is_running) return;
    post(_worker_threads, [this] {
        handle_jobs(true);
    });
}

void Server::add_client_deferred(tcp::socket&& socket) {
    static std::atomic<uint32_t> id_generator {0};

    try {
        socket.set_option(tcp::no_delay(true));
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Connection] Error setting no-delay: {}", e.what());
        return;
    }

    uint32_t id = ++id_generator;
    auto client = std::make_shared<Client>(_ctx, std::move(socket), id,
        [this](std::shared_ptr<Client>&& client, std::vector<uint8_t>&& buffer) {
            handle_packet(std::make_unique<Packet>(std::move(client), std::move(buffer)));
        }
    );

    _jobs.push([this, client = std::move(client)] {
        _clients.insert_or_assign(client->id, client);
        client->start();
        client->disconnected.subscribe(_on_client_disconnected->shared_from_this());

        spdlog::info("[Server] Added client ({})", client->id);
    });
}

void Server::remove_client_deferred(std::shared_ptr<Client>&& client) {
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
    case PacketType::ClientJoinRequest:
        handle_client_join_request(std::move(packet->client),
            packet_base->packet_base_as<ClientJoinRequest>());
        break;

    default:
        _zones[_clients_current_zone[packet->client->id]]->handle_packet_deferred(std::move(packet));
        break;
    }
}

void Server::handle_client_join_request(std::shared_ptr<Client>&& client, const ClientJoinRequest* request) {
    if (!request->username() || !request->token()) {
        client->stop();
        return;
    }

    const auto platform = platform_to_string(request->platform());
    const auto username = request->username()->string_view();
    const auto token = request->token()->string_view();


    bool verification_ok {true};
    try {
        const auto decoded_token = jwt::decode(token.data());
        const auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256 {Settings::auth_jwt_key().data()});

        verifier.verify(decoded_token);
    } catch (const std::invalid_argument&) {
        verification_ok = false;
    } catch (const std::runtime_error&) {
        verification_ok = false;
    } catch (const jwt::error::token_verification_error&) {
        verification_ok = false;
    }

    if (!verification_ok) {
        spdlog::info("[Server] [ClientJoinRequest] {}/{}: Invalid", platform, username);

        flatbuffers::FlatBufferBuilder builder {64};
        const auto client_join = CreateClientJoinResponse(builder, ClientJoinResult::INVALID_TOKEN);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse,
            client_join.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        client->stop();
        return;
    } else {
        spdlog::info("[Server] [ClientJoinRequest] {}/{}: OK", platform, username);
    }


    flatbuffers::FlatBufferBuilder builder {64};
    const auto client_join = CreateClientJoinResponse(builder, ClientJoinResult::OK);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, client_join.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

    // _jobs.push([this, client = std::move(client)]() mutable {
    //     //TODO: Find player's last stayed zone
    //     _clients_current_zone.insert_or_assign(client->id, 0);
    //     _zones[0]->add_client_deferred(std::move(client));
    // });
}

std::string_view platform_to_string(const ClientPlatform platform, const bool lower) {
    using namespace std::string_view_literals;

    if (platform == ClientPlatform::TEST) {
        return lower ? "test"sv : "TEST"sv;
    }
    if (platform == ClientPlatform::STEAM) {
        return lower ? "steam"sv : "STEAM"sv;
    }
    return ""sv;
}
}
