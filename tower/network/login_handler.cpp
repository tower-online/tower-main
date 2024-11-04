#include <boost/asio.hpp>
#include <jwt-cpp/jwt.h>
#include <spdlog/spdlog.h>
#include <tower/network/login_handler.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/system/settings.hpp>

namespace tower::network {
using namespace tower::network::packet;

LoginHandler::LoginHandler(const std::shared_ptr<ServerSharedState>& shared_state)
    : _shared_state {shared_state} {}

void LoginHandler::handle_login(Login login) {
    try {
        const auto decoded_token = jwt::decode(login.token.data());
        const auto verifier = jwt::verify()
                              .allow_algorithm(jwt::algorithm::hs256{Settings::auth_jwt_key().data()})
                              .with_claim("username", jwt::claim(std::string {login.username}));
        verifier.verify(decoded_token);

        login.client->is_authenticated = true;
        // spdlog::info("[Server] [ClientJoinRequest] {}/{}: Token OK", client->client_id, character_name);
    } catch (const std::exception&) {
        spdlog::warn("[LoginHandler] {}/{}: Token Invalid", login.client->client_id, login.character_name);

        login.client->stop();
        return;
    }

    _shared_state->db_pool.async_get_connection([this, login = std::move(login)](auto ec, auto conn) mutable {
        if (ec) {
            spdlog::error("[LoginHandler] Error getting connection");
            return;
        }

        co_spawn(_shared_state->server_executor, [this, login = std::move(login), conn = std::move(conn)] mutable -> boost::asio::awaitable<void> {
            co_await handle_login_internal(std::move(login), std::move(conn));
        }, boost::asio::detached);
    });
}

boost::asio::awaitable<void> LoginHandler::handle_login_internal(Login login, boost::mysql::pooled_connection conn) {
    auto& client {login.client};

    auto [ec, statement] = co_await conn->async_prepare_statement(
        "SELECT c.id FROM users u JOIN characters c ON c.user_id = u.id AND c.name = ? WHERE u.username = ?",
        as_tuple(boost::asio::use_awaitable));
    if (ec) {
        spdlog::error("[LoginHandler] Error preparing query: {}", ec.message());
        co_return;
    }

    boost::mysql::results result;
    if (const auto [ec] = co_await conn->async_execute(
            statement.bind(login.character_name, login.username), result, as_tuple(boost::asio::use_awaitable));
        ec) {
        spdlog::error("[LoginHandler] Error executing query: {}", ec.message());
        client->stop();
        co_return;
    }

    if (result.rows().empty()) {
        spdlog::warn(
            "[LoginHandler] Invalid username or character name: {}/{}", login.username, login.character_name);
        client->stop();
        co_return;
    }

    client->player = co_await player::Player::load(conn, login.character_name, client->client_id);
    if (!client->player) {
        spdlog::error("[LoginHandler] Error loading player: {}", login.character_name);
        client->stop();
        co_return;
    }
    spdlog::debug("Loaded player {}", client->player->name());

    flatbuffers::FlatBufferBuilder builder {1024};
    const auto spawn = CreatePlayerSpawn(builder,
        true, client->player->entity_id, client->client_id, client->player->write_player_info(builder));
    //TODO: Find player's last stayed zone. (Current: Default Zone 1)
    const WorldLocation current_location{1, 1};
    const auto response = CreateClientJoinResponse(builder, &current_location, spawn);
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, response.Union()));
    client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}
}
