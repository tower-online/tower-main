#include <jwt-cpp/jwt.h>
#include <tower/network/server.hpp>
#include <tower/player/player.hpp>
#include <tower/system/settings.hpp>

#include <tower/entity/simple_monster.hpp>

namespace tower::network {
Server::Server(boost::asio::any_io_executor&& executor, const std::shared_ptr<ServerSharedState>& shared_state) :
    _executor {std::move(executor)}, _strand {make_strand(_executor)}, _shared_state {shared_state} {
    _acceptor = std::make_unique<tcp::acceptor>(
        make_strand(_executor), tcp::endpoint {tcp::v4(), Settings::listen_port()});
    _acceptor->set_option(boost::asio::socket_base::reuse_address(true));
    _acceptor->listen(Settings::listen_backlog());
}

Server::~Server() { stop(); }

void Server::init() {
    // TODO: Read zone data from file or DB
    for (uint32_t zone_id {1}; zone_id <= 10; ++zone_id) {
        auto zone {Zone::create(
            zone_id,
            _executor,
            std::format("{}/zones/F1Z{}.bin", TOWER_DATA_ROOT, zone_id),
            _shared_state)};
        if (!zone) continue;

        _zones[zone_id] = std::move(zone);
    }

    // For test
    auto monster {SimpleMonster::create()};
    _zones.at(1)->subworld()->add_entity(monster);
}

void Server::start() {
    if (_is_running.exchange(true))
        return;
    spdlog::info("[Server] Starting...");

    // Spawn accepting loop
    co_spawn(_executor, [this] -> boost::asio::awaitable<void> {
        {
            const tcp::endpoint local_endpoint {_acceptor->local_endpoint()};
            spdlog::info("[Server] Accepting on {}:{}", local_endpoint.address().to_string(), local_endpoint.port());
        }
        while (_is_running) {
            auto [ec, socket] = co_await _acceptor->async_accept(as_tuple(boost::asio::use_awaitable));
            if (ec) {
                // Ignore
                if (ec == boost::asio::error::operation_aborted)
                    break;

                spdlog::error("Error accepting: {}", ec.message());
                continue;
            }
            try {
                const tcp::endpoint remote_endpoint {socket.remote_endpoint()};
                spdlog::info("[Server] Accepted {}:{}", remote_endpoint.address().to_string(), remote_endpoint.port());
            } catch (const boost::system::system_error&) {
                continue;
            }

            try {
                socket.set_option(tcp::no_delay(true));
            } catch (const boost::system::system_error& e) {
                spdlog::error("[Server] Error setting no-delay: {}", e.code().message());
                continue;
            }

            add_client_deferred(std::move(socket));
        }
    }, boost::asio::detached);


    // Spwan profiler logging loop
    co_spawn(_executor, [this]->boost::asio::awaitable<void>{
        boost::asio::steady_timer timer {_executor};

        while (_is_running) {
            timer.expires_after(3000ms);
            if (auto [ec] = co_await timer.async_wait(as_tuple(boost::asio::use_awaitable));
                ec || !_is_running) {
                co_return;
            }

            _profiler.renew();
            spdlog::info("{} clients, {} packets/s, {} bytes/s", _clients.size(), _profiler.get_pps(), _profiler.get_bps());
        }
    }, boost::asio::detached);
}

void Server::stop() {
    if (!_is_running.exchange(false)) return;
    spdlog::info("[Server] Stopping...");

    try {
        _acceptor->close();
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Server] Error closing acceptor: {}", e.code().message());
    }

    // Clean up above the strand
    std::promise<void> cleanup_promise {};
    auto cleanup_future {cleanup_promise.get_future()};
    post(_strand, [this, cleanup_promise = std::move(cleanup_promise)] mutable {
        for (auto& [_, client] : _clients) {
            client->stop();
        }

        for (auto& [_, zone] : _zones) {
            zone->stop();
        }
        _zones.clear();

        cleanup_promise.set_value();
    });
    cleanup_future.wait();
}

void Server::add_client_deferred(tcp::socket&& socket) {
    static uint32_t client_id_generator {0};
    auto client = std::make_shared<Client>(
        _executor,
        std::move(socket),
        ++client_id_generator,
        [this](std::shared_ptr<Client>&& c, std::vector<uint8_t>&& buffer) {
            _profiler.add_packet(buffer.size());

            if (flatbuffers::Verifier verifier {buffer.data(), buffer.size()}; !VerifyPacketBaseBuffer(verifier)) {
                spdlog::warn("[Server] Invalid PacketBase from Client({})", c->client_id);
                c->stop();
                return;
            }

            // Sequentialize packet handling on the strand
            auto packet = std::make_unique<Packet>(std::move(c), std::move(buffer));
            co_spawn(_strand, [this, packet = std::move(packet)] mutable -> boost::asio::awaitable<void> {
                co_await handle_packet(std::move(packet));
            }, boost::asio::detached);
        },
        [this](std::shared_ptr<Client>&& c) {
            remove_client_deferred(std::move(c));
        });

    post(_strand, [this, client = std::move(client)] mutable {
        const auto client_id = client->client_id;
        _clients.emplace(client_id, std::move(client));
        _clients_current_zone.emplace(client_id, 0);

        _clients.at(client_id)->start();
    });
}

void Server::remove_client_deferred(std::shared_ptr<Client> client) {
    post(_strand, [this, client = std::move(client)] {
        client->stop();
        
        const auto current_zone {_clients_current_zone.at(client->client_id)};
        if (_zones.contains(current_zone)) {
            _zones.at(current_zone)->remove_client_deferred(client);
        }

        _clients.erase(client->client_id);
        _clients_current_zone.erase(client->client_id);
        spdlog::info("[Server] Removed Client({})", client->client_id);
    });
}

boost::asio::awaitable<void> Server::handle_packet(std::shared_ptr<Packet>&& packet) {
    switch (const auto packet_base = GetPacketBase(packet->buffer.data()); packet_base->packet_base_type()) {
    case PacketType::ClientJoinRequest:
        co_await handle_client_join_request(
            std::move(packet->client), packet_base->packet_base_as<ClientJoinRequest>());
        break;

    case PacketType::PlayerEnterZoneRequest:
        handle_player_enter_zone_request(
            std::move(packet->client), packet_base->packet_base_as<PlayerEnterZoneRequest>());
        break;

    case PacketType::HeartBeat:
        // Ignore, because the client already heart-beated by itself
        break;

    case PacketType::Ping: {
        flatbuffers::FlatBufferBuilder builder {64};
        const auto ping = packet_base->packet_base_as<Ping>();
        const auto pong = CreatePing(builder, ping->seq());
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::Ping, pong.Union()));
        packet->client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        break;
    }

    default:
        if (!packet->client->is_authenticated) {
            spdlog::warn("[Server] Packet from unauthenticated client");
            packet->client->stop();
            co_return;
        }

        const auto client_id = packet->client->client_id;
        if (!_clients.contains(client_id)) co_return;
        _zones.at(_clients_current_zone.at(client_id))->handle_packet_deferred(std::move(packet));

        break;
    }
}

boost::asio::awaitable<void> Server::handle_client_join_request(
    std::shared_ptr<Client>&& client, const ClientJoinRequest* request) {
    if (!request->username() || !request->character_name() || !request->token()) {
        spdlog::warn("[Server] [ClientJoinRequest] ({}): Invalid request", client->client_id);
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
        // spdlog::info("[Server] [ClientJoinRequest] {}/{}: Token OK", client->client_id, character_name);
    } catch (const std::exception&) {
        spdlog::warn("[Server] [ClientJoinRequest] {}/{}: Token Invalid", client->client_id, character_name);

        client->stop();
        co_return;
    }

    {
        static std::atomic<int> reach_count {0};

        reach_count += 1;
        spdlog::debug("reach count: {}", reach_count.load());

        boost::mysql::diagnostics diag;
        auto [ec, conn] {co_await _shared_state->db_pool.async_get_connection(diag, as_tuple(boost::asio::use_awaitable))};
        try {
            boost::mysql::throw_on_error(ec, diag);
        } catch (const std::exception& e) {
            spdlog::error("Error getting connection: {}", ec.message());
            client->stop();

            reach_count -= 1;
            co_return;
        }

        reach_count -= 1;

        {
            auto [ec, statement] = co_await conn->async_prepare_statement(
                "SELECT c.id FROM users u JOIN characters c ON c.user_id = u.id AND c.name = ? WHERE u.username = ?",
                as_tuple(boost::asio::use_awaitable));
            if (ec) {
                spdlog::error("[Server] [ClientJoinRequest] Error preparing query: {}", ec.message());
                co_return;
            }

            boost::mysql::results result;
            if (const auto [ec] = co_await conn->async_execute(
                    statement.bind(character_name, username), result, as_tuple(boost::asio::use_awaitable));
                ec) {
                spdlog::error("[Server] [ClientJoinRequest] Error executing query: {}", ec.message());
                client->stop();
                co_return;
                }

            if (result.rows().empty()) {
                spdlog::warn(
                    "[Server] [ClientJoinRequest] Invalid username or character name: {}/{}", username, character_name);
                client->stop();
                co_return;
            }
        }

        client->player = co_await player::Player::load(conn, character_name);
        if (!client->player) {
            spdlog::error("[Server] [ClientJoinRequest] Error loading player: {}", character_name);
            client->stop();
            co_return;
        }
    }

    flatbuffers::FlatBufferBuilder builder {1024};
    const auto spawn = CreatePlayerSpawn(builder,
        true, client->player->entity_id, client->player->write_player_info(builder));
    //TODO: Find player's last stayed zone. (Current: Default Zone 1)
    const WorldLocation current_location {1, 1};
    const auto response = CreateClientJoinResponse(builder, &current_location, spawn);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, response.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

    // _clients_current_zone[client->client_id] = 1;
}

void Server::handle_player_enter_zone_request(std::shared_ptr<Client>&& client, const PlayerEnterZoneRequest* request) {
    //TODO: Check if zone is reachable from current player's location
    const auto location = request->location();
    auto& current_zone_id {_clients_current_zone.at(client->client_id)};
    const auto next_zone_id {location->zone_id()};
    const bool is_first_enter {current_zone_id == 0};

    if (current_zone_id == next_zone_id) {
        spdlog::warn("[Server] Client({}): Requested entering to current zone", client->client_id);
        return;
    }

    if ((is_first_enter || _zones.contains(current_zone_id)) && _zones.contains(next_zone_id)) {
        //TODO: Player can be exist on two zones at the same time... fix?
        if (!is_first_enter) {
            _zones.at(current_zone_id)->remove_client_deferred(client);
        }
        _zones.at(next_zone_id)->add_client_deferred(client);
    } else {
        spdlog::warn("[Server] Client({}): Requested invalid zone enter", client->client_id);
        client->stop();
        return;
    }
    current_zone_id = next_zone_id;

    flatbuffers::FlatBufferBuilder builder {128};
    const auto response = CreatePlayerEnterZoneResponse(builder, true, request->location());
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerEnterZoneResponse, response.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}
}
