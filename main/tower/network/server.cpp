#include <jwt-cpp/jwt.h>
#include <tower/network/db.hpp>
#include <tower/network/server.hpp>
#include <tower/player/player.hpp>

#include <cassert>

namespace tower::network {
Server::Server(const size_t num_workers)
    : _workers {num_workers}, _num_workers {num_workers} {
    spdlog::info("[Server] {} worker threads", _num_workers);
}

Server::~Server() {
    stop();

    _workers.join();
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


    // default zone
    _zones.insert_or_assign(0, std::make_unique<Zone>(0, _ctx));
    _zones[0]->init("test_zone");
}

void Server::start() {
    if (_is_running.exchange(true)) return;
    spdlog::info("[Server] Starting...");

    _listener.start();

    //TODO: context_guard?
    for (auto i {0}; i < _num_workers; ++i) {
        post(_workers, [this] {
            while (_is_running) {
                _ctx.run();
            }
        });
    }

    _workers.join();
}

void Server::stop() {
    if (!_is_running.exchange(false)) return;
    spdlog::info("[Server] Stopping...");

    for (auto& [_, client] : _clients) {
        client->stop();
    }

    for (auto& [_, zone] : _zones) {
        zone->stop();
    }

    _listener.stop();
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

    co_spawn(_jobs_strand, [this, client = std::move(client)]()->boost::asio::awaitable<void> {
        _clients[client->id] = client;
        _clients_on_disconnected[client->id] = client->disconnected.connect(
            [this](std::shared_ptr<Client> disconnecting_client) {
                remove_client_deferred(std::move(disconnecting_client));
            });
        spdlog::info("[Server] Added client ({})", client->id);

        client->start();
        co_return;
    }, boost::asio::detached);
}

void Server::remove_client_deferred(std::shared_ptr<Client>&& client) {
    co_spawn(_jobs_strand, [this, client = std::move(client)]()->boost::asio::awaitable<void> {
        _clients.erase(client->id);
        _clients_on_disconnected.erase(client->id);
        _clients_current_zone.erase(client->id);
        spdlog::info("[Server] Removed client ({})", client->id);

        co_return;
    }, boost::asio::detached);
}

void Server::handle_packet(std::unique_ptr<Packet> packet) {
    if (flatbuffers::Verifier verifier {packet->buffer.data(), packet->buffer.size()}; !
        VerifyPacketBaseBuffer(verifier)) {
        spdlog::warn("[Server] Invalid PacketBase from client({})", packet->client->id);
        packet->client->stop();
        return;
    }

    std::shared_ptr<Packet> shared_packet = std::move(packet);
    co_spawn(_jobs_strand, [this, packet = std::move(shared_packet)]() mutable ->boost::asio::awaitable<void> {
        co_await handle_packet_internal(std::move(packet));
    }, boost::asio::detached);
}

boost::asio::awaitable<void> Server::handle_packet_internal(std::shared_ptr<Packet>&& packet) {
    switch (const auto packet_base = GetPacketBase(packet->buffer.data()); packet_base->packet_base_type()) {
    case PacketType::ClientJoinRequest:
        co_await handle_client_join_request(
            std::move(packet->client), packet_base->packet_base_as<ClientJoinRequest>());
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

        if (!_clients_current_zone.contains(packet->client->id)) co_return;
        _zones[_clients_current_zone[packet->client->id]]->handle_packet_deferred(std::move(packet));

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

    try {
        const auto decoded_token = jwt::decode(token.data());
        const auto verifier = jwt::verify()
                              .allow_algorithm(jwt::algorithm::hs256 {Settings::auth_jwt_key().data()})
                              .with_claim("username", jwt::claim(std::string {username}));
        verifier.verify(decoded_token);

        client->is_authenticated = true;
        spdlog::info("[Server] [ClientJoinRequest] {}/{}: Token OK", client->id, character_name);
    } catch (const std::exception&) {
        spdlog::warn("[Server] [ClientJoinRequest] {}/{}: Token Invalid", client->id, character_name);

        client->stop();
        co_return;
    }

    // client->stop();
    // co_return;
    {
        {
            auto conn = co_await DB::get_connection();

            auto [ec, statement] = co_await conn->async_prepare_statement(
                "SELECT c.id FROM users u JOIN characters c ON c.user_id = u.id AND c.name = ? WHERE u.username = ?",
                as_tuple(boost::asio::use_awaitable));
            if (ec) {
                spdlog::error("[Server] [ClientJoinRequest] Error preparing query: {}", ec.message());
                co_return;
            }

            boost::mysql::results result;
            if (const auto [ec] = co_await conn->async_execute(
                statement.bind(character_name, username), result, as_tuple(boost::asio::use_awaitable)); ec) {
                spdlog::error("[Server] [ClientJoinRequest] Error executing query: {}", ec.message());
                client->stop();
                co_return;
            }

            if (result.rows().empty()) {
                spdlog::warn("[Server] [ClientJoinRequest] Invalid username or character name: {}/{}", username,
                    character_name);
                client->stop();
                co_return;
            }
        }

        // client->player = co_await player::Player::load(character_name);
        if (!client->player) {
            spdlog::error("[Server] [ClientJoinRequest] Error loading player: {}", character_name);
            client->stop();
            co_return;
        }

        const auto& stats = client->player->stats;

        flatbuffers::FlatBufferBuilder builder {1024};

        std::vector<PlayerStat> optional_stats {};
        for (const auto& [_, stat] : stats.optionals) {
            optional_stats.emplace_back(stat.type(), stat.get());
        }
        const auto stats_offset = CreatePlayerStatsDirect(builder,
            stats.level.get(), stats.exp.get(), stats.str.get(), stats.mag.get(), stats.agi.get(), stats.con.get(),
            &optional_stats);

        const auto info_offset = CreatePlayerInfoDirect(builder, client->player->entity_type,
            client->player->name().data(), stats_offset);

        const auto spawn_offset = CreatePlayerSpawn(builder, true, client->player->entity_id, info_offset);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerSpawn, spawn_offset.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    }

    // _jobs.push([this, client = std::move(client)]() mutable {
    //     //TODO: Find player's last stayed zone. (Current: Default Zone 0)
    //     _clients_current_zone[client->id] = 0;
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
