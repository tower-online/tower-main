#pragma once

#include <tower/net/connection.hpp>
#include <tower/player/player.hpp>
#include <tower/system/event.hpp>

namespace tower::net {
using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id,
        std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)>&& packet_received);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void start();
    void stop();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);

public:
    const uint32_t id;
    std::shared_ptr<player::Player> player {player::Player::create()};
    Event<std::shared_ptr<Client>> disconnected {};

private:
    Connection _connection;
    const std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)> _packet_received;
    std::atomic<bool> _is_running {false};
};
}
