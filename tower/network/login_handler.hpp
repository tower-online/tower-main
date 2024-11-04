#pragma once

#include <boost/mysql.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/server_shared_state.hpp>

namespace tower::network {
class LoginHandler {
public:
    struct Login {
        Login(std::string_view username, std::string_view character_name, std::string_view token, std::unique_ptr<Packet> packet)
            : username {username}, character_name {character_name}, token {token},
            client {packet->client}, _packet {std::move(packet)} {}

        std::string_view username;
        std::string_view character_name;
        std::string_view token;
        std::shared_ptr<Client>& client;

    private:
        // For holding lifetime of fields
        std::unique_ptr<Packet> _packet;
    };

    explicit LoginHandler(const std::shared_ptr<ServerSharedState>& shared_state);

    void handle_login(Login login);
    static boost::asio::awaitable<void> handle_login_internal(Login login, boost::mysql::pooled_connection conn);

private:
    std::shared_ptr<ServerSharedState> _shared_state;
};
}