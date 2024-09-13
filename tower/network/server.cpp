#include <jwt-cpp/jwt.h>
#include <tower/network/server.hpp>
#include <tower/player/player.hpp>
#include <tower/system/settings.hpp>

namespace tower::network {
Server::Server(boost::asio::any_io_executor&& executor, const std::shared_ptr<ServerSharedState>& st) :
    _executor {std::move(executor)}, _strand {make_strand(_executor)}, _st {st} {
    _acceptor = std::make_unique<tcp::acceptor>(
        make_strand(_executor), tcp::endpoint {tcp::v4(), Settings::main_listen_port()});
    _acceptor->set_option(boost::asio::socket_base::reuse_address(true));
    _acceptor->listen(Settings::main_listen_backlog());
}

Server::~Server() { stop(); }

void Server::init() {
    // default zone
    _zones.insert_or_assign(0, std::make_unique<Zone>(0, _executor));
    _zones[0]->init("test_zone");
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

            static uint32_t entry_id_generator {0};
            auto client = std::make_shared<Client>(_executor,
                std::move(socket),
                ++entry_id_generator,
                [this](std::shared_ptr<Client>&& c, std::vector<uint8_t>&& buffer) {
                if (flatbuffers::Verifier verifier {buffer.data(), buffer.size()}; !VerifyPacketBaseBuffer(verifier)) {
                    spdlog::warn("[Server] Invalid PacketBase from Client({})", c->entry_id);
                    c->stop();
                    return;
                }

                auto packet = std::make_unique<Packet>(std::move(c), std::move(buffer));
                co_spawn(_strand, [this, packet = std::move(packet)] mutable -> boost::asio::awaitable<void> {
                    co_await handle_packet(std::move(packet));
                }, boost::asio::detached);
            });
            auto entry = std::make_unique<ClientEntry>(
                client, client->disconnected.connect([this](const std::shared_ptr<Client>& c) {
                post(_strand, [this, c] {
                    c->stop();
                    _client_entries.erase(c->entry_id);
                    spdlog::info("[Server] Removed Client({})", c->entry_id);
                });
            }));

            post(_strand, [this, entry = std::move(entry)] mutable {
                const auto entry_id = entry->client->entry_id;
                _client_entries.emplace(entry_id, std::move(entry));
                _client_entries[entry_id]->client->start();
            });
        }
    }, boost::asio::detached);
}

void Server::stop() {
    if (!_is_running.exchange(false))
        return;
    spdlog::info("[Server] Stopping...");

    try {
        _acceptor->close();
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Server] Error closing acceptor: {}", e.code().message());
    }

    for (auto& [_, entry] : _client_entries) {
        entry->client->stop();
    }
    // TODO: clear _clients -> is safe? -> cancellation token?

    for (auto& [_, zone] : _zones) {
        zone->stop();
    }
    // _zones.clear();
}

boost::asio::awaitable<void> Server::handle_packet(std::shared_ptr<Packet>&& packet) {
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
            spdlog::warn("[Server] Packet from unauthenticated client");
            packet->client->stop();
            co_return;
        }

        const auto entry_id = packet->client->entry_id;
        if (!_client_entries.contains(entry_id))
            co_return;
        _zones.at(_client_entries.at(entry_id)->current_zone_id)->handle_packet_deferred(std::move(packet));

        break;
    }
}

boost::asio::awaitable<void> Server::handle_client_join_request(
    std::shared_ptr<Client>&& client, const ClientJoinRequest* request) {
    if (!request->username() || !request->character_name() || !request->token()) {
        spdlog::warn("[Server] [ClientJoinRequest] ({}): Invalid request", client->entry_id);
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
        spdlog::info("[Server] [ClientJoinRequest] {}/{}: Token OK", client->entry_id, character_name);
    } catch (const std::exception&) {
        spdlog::warn("[Server] [ClientJoinRequest] {}/{}: Token Invalid", client->entry_id, character_name);

        client->stop();
        co_return;
    }

    {
        auto conn = co_await _st->db_pool.async_get_connection(boost::asio::use_awaitable);
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

    const auto& stats = client->player->stats;

    flatbuffers::FlatBufferBuilder builder {1024};
    const auto spawn = CreatePlayerSpawn(builder,
        true, client->player->entity_id, client->player->write_player_info(builder));
    //TODO: Find player's last stayed zone. (Current: Default Zone 0)
    const WorldLocation current_location {1, 0};
    const auto response = CreateClientJoinResponse(builder, &current_location, spawn);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, response.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
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
} // namespace tower::network
