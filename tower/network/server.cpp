#include <tower/network/server.hpp>
#include <tower/player/player.hpp>
#include <tower/system/settings.hpp>

#include <tower/entity/simple_monster.hpp>

namespace tower::network {
Server::Server(const std::shared_ptr<ServerSharedState>& shared_state) :
    _shared_state {shared_state}, _login_handler {_shared_state} {
    _acceptor = std::make_unique<tcp::acceptor>(
        make_strand(_shared_state->server_executor), tcp::endpoint{tcp::v4(), Settings::listen_port()});
    _acceptor->set_option(boost::asio::socket_base::reuse_address(true));
    _acceptor->listen(Settings::listen_backlog());
}

Server::~Server() { stop(); }

void Server::init() {
    // TODO: Read zone data from file or DB
    for (uint32_t zone_id{1}; zone_id <= 10; ++zone_id) {
        auto zone{Zone::create(
            zone_id,
            _shared_state->server_executor,
            std::format("{}/zones/F1Z{}.bin", TOWER_DATA_ROOT, zone_id),
            _shared_state)};
        if (!zone)
            continue;

        _zones[zone_id] = std::move(zone);
    }
}

void Server::start() {
    if (_is_running.exchange(true))
        return;
    spdlog::info("[Server] Starting...");

    // Spawn accepting loop
    co_spawn(_shared_state->server_executor, [this] -> boost::asio::awaitable<void> {
        {
            const tcp::endpoint local_endpoint{_acceptor->local_endpoint()};
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
                const tcp::endpoint remote_endpoint{socket.remote_endpoint()};
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
    co_spawn(_shared_state->server_executor, [this]-> boost::asio::awaitable<void> {
        boost::asio::steady_timer timer {_shared_state->server_executor};

        while (_is_running) {
            timer.expires_after(3000ms);
            if (auto [ec] = co_await timer.async_wait(as_tuple(boost::asio::use_awaitable));
                ec || !_is_running) {
                co_return;
            }

            _profiler.renew();
            spdlog::info("{} clients, {} packets/s, {} bytes/s", _clients.size(), _profiler.get_pps(),
                _profiler.get_bps());
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

    // Clean up above the strand
    std::promise<void> cleanup_promise{};
    auto cleanup_future{cleanup_promise.get_future()};
    post(_shared_state->server_strand, [this, cleanup_promise = std::move(cleanup_promise)] mutable {
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
    static uint32_t client_id_generator{0};
    auto client = std::make_shared<Client>(
        _shared_state->server_executor,
        std::move(socket),
        ++client_id_generator,
        [this](std::shared_ptr<Client>&& c, std::vector<uint8_t>&& buffer) {
            _profiler.add_packet(buffer.size());

            if (flatbuffers::Verifier verifier{buffer.data(), buffer.size()}; !VerifyPacketBaseBuffer(verifier)) {
                spdlog::warn("[Server] Invalid PacketBase from Client({})", c->client_id);
                c->stop();
                return;
            }

            // Sequentialize packet handling on the strand
            auto packet = std::make_unique<Packet>(std::move(c), std::move(buffer));
            post(_shared_state->server_strand, [this, packet = std::move(packet)] mutable {
                handle_packet(std::move(packet));
            });
        },
        [this](std::shared_ptr<Client>&& c) {
            remove_client_deferred(std::move(c));
        });

    post(_shared_state->server_strand, [this, client = std::move(client)] mutable {
        const auto client_id = client->client_id;
        _clients.emplace(client_id, std::move(client));
        _clients_current_zone.emplace(client_id, 0);

        _clients.at(client_id)->start();
    });
}

void Server::remove_client_deferred(std::shared_ptr<Client> client) {
    post(_shared_state->server_strand, [this, client = std::move(client)] {
        client->stop();

        _shared_state->party_manager.remove_client(client->client_id);

        const auto current_zone{_clients_current_zone.at(client->client_id)};
        if (_zones.contains(current_zone)) {
            _zones.at(current_zone)->remove_client_deferred(client);
        }

        _clients.erase(client->client_id);
        _clients_current_zone.erase(client->client_id);
        spdlog::info("[Server] Removed Client({})", client->client_id);
    });
}

void Server::broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer) {
    for (const auto& [_, client] : _clients) {
        client->send_packet(buffer);
    }
}

void Server::handle_packet(std::unique_ptr<Packet> packet) {
    switch (const auto packet_base = GetPacketBase(packet->buffer.data()); packet_base->packet_base_type()) {
    case PacketType::ClientJoinRequest:
        handle_client_join_request(
            std::move(packet), packet_base->packet_base_as<ClientJoinRequest>());
        break;

    case PacketType::PlayerEnterZoneRequest:
        handle_player_enter_zone_request(
            std::move(packet->client), packet_base->packet_base_as<PlayerEnterZoneRequest>());
        break;

    case PacketType::PlayerJoinPartyRequest:
        handle_player_join_party_request(
            std::move(packet->client), packet_base->packet_base_as<PlayerJoinPartyRequest>());
        break;

    case PacketType::PlayerJoinPartyResponse:
        handle_player_join_party_response(
            std::move(packet->client), packet_base->packet_base_as<PlayerJoinPartyResponse>());
        break;

    case PacketType::PlayerLeaveParty:
        handle_player_leave_party(
            std::move(packet->client), packet_base->packet_base_as<PlayerLeaveParty>());
        break;

    case PacketType::PlayerChat:
        handle_player_chat(
            std::move(packet->client), packet_base->packet_base_as<PlayerChat>(), packet->buffer.size());
        break;

    case PacketType::HeartBeat:
        // Ignore, because the client already heart-beated by itself
        break;

    case PacketType::Ping: {
        flatbuffers::FlatBufferBuilder builder{64};
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
            return;
        }

        const auto client_id = packet->client->client_id;
        if (!_clients.contains(client_id))
            return;
        _zones.at(_clients_current_zone.at(client_id))->handle_packet_deferred(std::move(packet));

        break;
    }
}

void Server::handle_client_join_request(std::unique_ptr<Packet> packet, const ClientJoinRequest* request) {
    if (!request->username() || !request->character_name() || !request->token()) {
        spdlog::warn("[Server] [ClientJoinRequest] ({}): Invalid request", packet->client->client_id);
        packet->client->stop();
        return;
    }

    _login_handler.handle_login(LoginHandler::Login {
        request->username()->string_view(),
        request->character_name()->string_view(),
        request->token()->string_view(),
        std::move(packet)
    });
}

void Server::handle_player_enter_zone_request(std::shared_ptr<Client>&& client, const PlayerEnterZoneRequest* request) {
    //TODO: Check if zone is reachable from current player's location
    const auto location = request->location();
    auto& current_zone_id{_clients_current_zone.at(client->client_id)};
    const auto next_zone_id{location->zone_id()};
    const bool is_first_enter{current_zone_id == 0};

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

    flatbuffers::FlatBufferBuilder builder{128};
    const auto response = CreatePlayerEnterZoneResponse(builder, true, request->location());
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerEnterZoneResponse, response.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Server::handle_player_join_party_request(std::shared_ptr<Client>&& client, const PlayerJoinPartyRequest* request) {
    const auto requester_id{request->requester()}, requestee_id{request->requestee()};
    if (client->client_id != requester_id)
        return;
    if (!_clients.contains(requestee_id))
        return;

    // Ask for an approval
    _shared_state->party_manager.pending_requests.insert_or_assign(
        requester_id, std::make_shared<PartyManager::PendingRequest>(requestee_id));

    flatbuffers::FlatBufferBuilder builder{128};
    const auto ask = CreatePlayerJoinPartyRequest(builder, requester_id, requestee_id);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerJoinPartyRequest, ask.Union()));
    _clients.at(requestee_id)->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Server::handle_player_join_party_response(std::shared_ptr<Client>&& client,
    const PlayerJoinPartyResponse* response) {
    const auto requester_id{response->requester()}, requestee_id{response->requestee()};
    if (client->client_id != requestee_id)
        return;
    if (!_clients.contains(requester_id))
        return;
    if (response->result() != PlayerJoinPartyResult::OK)
        return;

    // Check if request is still valid
    auto& party_manager {_shared_state->party_manager};
    std::shared_ptr<PartyManager::PendingRequest> request;
    if (!party_manager.pending_requests.try_at(requester_id, request))
        return;
    if (request->requestee_id != requestee_id)
        return;
    if (!request->is_valid()) {
        party_manager.pending_requests.erase(requester_id);
        return;
    }
    party_manager.pending_requests.erase(requester_id);

    // Join and notify the approval
    if (const auto current_party_id{party_manager.get_current_party_id(requestee_id)}; !current_party_id) {
        // Create a party if not exist
        const auto new_party_id{party_manager.add_party()};
        party_manager.add_member(new_party_id, _clients.at(requester_id));
        party_manager.add_member(new_party_id, _clients.at(requestee_id));
    } else {
        party_manager.add_member(*current_party_id, _clients.at(requester_id));
    }

    flatbuffers::FlatBufferBuilder builder{128};
    const auto response_relay{CreatePlayerJoinPartyResponse(builder,
        requester_id, requestee_id, PlayerJoinPartyResult::OK)};
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerJoinPartyResponse, response_relay.Union()));
    _clients.at(requester_id)->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Server::handle_player_leave_party(std::shared_ptr<Client>&& client, const PlayerLeaveParty* response) {
    const auto leaver_id {response->leaver_id()};
    if (client->client_id != leaver_id) return;

    const auto party_id {_shared_state->party_manager.remove_client(leaver_id)};
    if (!party_id) return;

    flatbuffers::FlatBufferBuilder builder{128};
    const auto leave {CreatePlayerLeaveParty(builder, leaver_id, PlayerLeavePartyReason::REQUESTED)};
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::PlayerLeaveParty, leave.Union()));

    _shared_state->party_manager.broadcast(*party_id, std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Server::handle_player_chat(std::shared_ptr<Client>&& client, const PlayerChat* chat, const size_t buffer_size) {
    const auto target {chat->target()};
    const auto target_id {chat->target_id()};
    const auto message {chat->message()};

    std::shared_ptr<flatbuffers::DetachedBuffer> buffer;
    {
        flatbuffers::FlatBufferBuilder builder {buffer_size + sizeof(flatbuffers::uoffset_t)};
        const auto echo {CreatePlayerChatDirect(builder,
            target, client->client_id, client->player->name().data(), message->data())};
        const auto packetBase {CreatePacketBase(builder, PacketType::PlayerChat, echo.Union())};
        builder.FinishSizePrefixed(packetBase);
        buffer = std::make_shared<flatbuffers::DetachedBuffer>(builder.Release());
    }

    if (target == PlayerChatTarget::SERVER) {
        broadcast(std::move(buffer));
    } else if (target == PlayerChatTarget::ZONE) {
        const auto current_zone_id {_clients_current_zone.at(client->client_id)};
        if (!_zones.contains(current_zone_id)) return;

        _zones.at(current_zone_id)->broadcast(std::move(buffer));
    } else if (target == PlayerChatTarget::GUILD) {
        // TODO
    } else if (target == PlayerChatTarget::PARTY) {
        auto& party_manager {_shared_state->party_manager};
        const auto party_id {party_manager.get_current_party_id(client->client_id)};
        if (!party_id) return;

        party_manager.broadcast(*party_id, buffer);
    } else if (target == PlayerChatTarget::WHISPER) {
        if (!_clients.contains(target_id)) return;

        _clients.at(target_id)->send_packet(std::move(buffer));
    }
}
}
