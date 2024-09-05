#include <jwt-cpp/jwt.h>
#include <tower/network/db.hpp>
#include <tower/network/server.hpp>

#include <cassert>

namespace tower::network {
Server::Server(const size_t num_io_threads, const size_t num_worker_threads)
    : _num_io_threads {num_io_threads}, _worker_threads {num_worker_threads} {
    // Worker threads must be greater than 1, because one of them is permanently dedicated to server.
    assert(num_worker_threads > 1);

    // default zone
    _zones.insert_or_assign(0, std::make_unique<Zone>(0, _worker_threads));
    _zones[0]->init("test_zone");
}

Server::~Server() {
    stop();

    _io_threads.join_all();
    _worker_threads.join();
}

void Server::init() {
    boost::mysql::pool_params params {};
    params.server_address.emplace_host_and_port(Settings::db_host().data(), Settings::db_port());
    params.username = Settings::db_user();
    params.password = Settings::db_password();
    params.database = Settings::db_name();

    DB::init(_ctx, std::move(params));

    redis::config redis_config {};
    redis_config.addr.host = Settings::redis_host();
    redis_config.password = Settings::redis_password();
    _redis_connection.async_run(redis_config, {}, boost::asio::detached);
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
        while (_is_running) {
            std::queue<std::function<void()>> jobs;
            _jobs.swap(jobs);

            while (!jobs.empty()) {
                const auto& job = jobs.front();
                job();
                jobs.pop();
            }
        }
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
        [this](std::shared_ptr<Client>&& client, std::vector<uint8_t>&& buffer)->boost::asio::awaitable<void> {
            co_await handle_packet(std::make_unique<Packet>(std::move(client), std::move(buffer)));
        }
    );

    _jobs.push([this, client = std::move(client)] {
        _clients[client->id] = client;
        _clients_on_disconnected[client->id] = client->disconnected.connect(
            [this](std::shared_ptr<Client> disconnecting_client) {
                remove_client_deferred(std::move(disconnecting_client));
            });
        spdlog::info("[Server] Added client ({})", client->id);

        client->start();
    });
}

void Server::remove_client_deferred(std::shared_ptr<Client>&& client) {
    _jobs.push([this, client = std::move(client)] {
        _clients.erase(client->id);
        _clients_on_disconnected.erase(client->id);
        _clients_current_zone.erase(client->id);
        spdlog::info("[Server] Removed client ({})", client->id);
    });
}

boost::asio::awaitable<void> Server::handle_packet(std::unique_ptr<Packet> packet) {
    if (flatbuffers::Verifier verifier {packet->buffer.data(), packet->buffer.size()}; !
        VerifyPacketBaseBuffer(verifier)) {
        spdlog::warn("[Server] Invalid PacketBase from client({})", packet->client->id);
        packet->client->stop();
        co_return;
    }

    switch (const auto packet_base = GetPacketBase(packet->buffer.data()); packet_base->packet_base_type()) {
    case PacketType::ClientJoinRequest:
        co_await handle_client_join_request(std::move(packet->client),
            packet_base->packet_base_as<ClientJoinRequest>());
        break;

    case PacketType::HeartBeat:
        // Ignore, because the client already heart-beated by itself
        break;

    default:
        if (!packet->client->is_authenticated) {
            spdlog::warn("[Server] Packet from unauthenticated client({})", packet->client->id);
            packet->client->stop();
            co_return;
        }

    // Serialize handing over jobs, because accessing to zones is critical section.
        std::shared_ptr<Packet> shared_packet = std::move(packet);
        _jobs.push([this, shared_packet = std::move(shared_packet)]() mutable ->void {
            if (!_clients_current_zone.contains(shared_packet->client->id)) return;
            _zones[_clients_current_zone[shared_packet->client->id]]->handle_packet_deferred(std::move(shared_packet));
        });
        break;
    }
}

boost::asio::awaitable<void> Server::handle_client_join_request(std::shared_ptr<Client>&& client,
    const ClientJoinRequest* request) {
    if (!request->username() || !request->character_name() || !request->token()) {
        spdlog::warn("[Server] [ClientJoinRequest] {}: Invalid request", client->id);
        client->stop();
        co_return;
    }

    const auto username = request->username()->string_view();
    const auto character_name = request->character_name()->string_view();
    const auto token = request->token()->string_view();

    bool verification_ok {true};
    try {
        const auto decoded_token = jwt::decode(token.data());
        const auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256 {Settings::auth_jwt_key().data()});

        verifier.verify(decoded_token);

        //TODO: Check username and store
        // client->username = request->username()

        client->is_authenticated = true;
        spdlog::info("[Server] [ClientJoinRequest] {}/{}: Token OK", client->id, character_name);
    } catch (const std::exception&) {
        verification_ok = false;
        spdlog::warn("[Server] [ClientJoinRequest] {}/{}: Token Invalid", client->id, character_name);
    }

    {
        flatbuffers::FlatBufferBuilder builder {64};
        const auto client_join = CreateClientJoinResponse(builder,
            verification_ok ? ClientJoinResult::OK : ClientJoinResult::INVALID_TOKEN);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, client_join.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }
    if (!verification_ok) {
        client->stop();
        co_return;
    }

    {
        auto conn = co_await DB::get_connection();

        auto [ec, statement] = co_await conn->async_prepare_statement(
            "SELECT name, race FROM users u JOIN characters c ON c.user_id = u.id AND c.name = ? WHERE u.username = ?",
            as_tuple(boost::asio::use_awaitable));
        if (ec) {
            spdlog::error("[Server] [ClientJoinRequest] Error preparing query");
            co_return;
        }

        boost::mysql::results result;
        if (const auto [ec] = co_await conn->async_execute(
            statement.bind(character_name), result, as_tuple(boost::asio::use_awaitable)); ec || result.empty()) {
            spdlog::error("[Server] [ClientJoinRequest] Error executing query or empty result");
            client->stop();
            co_return;
        }

        std::string race;
        for (const auto& r : result.rows()) {
            race = r.at(1).as_string();
            if (!entity_types_map.contains(race)) {
                spdlog::error("[Server] [ClientJoinRequest] Invalid character race: {}", race);
                co_return;
            }

            //TODO: Setup client->player
            break;
        }

        flatbuffers::FlatBufferBuilder builder {128};
        const auto spawn = CreatePlayerSpawn(builder, entity_types_map[race], client->player->entity_id);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawn, spawn.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    _jobs.push([this, client = std::move(client)]() mutable {
        //TODO: Find player's last stayed zone. (Current: Default Zone 0)
        _clients_current_zone[client->id] = 0;
        _zones[0]->add_client_deferred(std::move(client));
    });
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
