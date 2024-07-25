#pragma once

#include <tower/game/player/player.hpp>
#include <tower/net/connection.hpp>
#include <tower/net/packet.hpp>

namespace tower::net {
using boost::asio::ip::tcp;
using namespace game::player;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id,
        std::function<void(std::shared_ptr<Client>)>&& disconnected,
        std::function<void(std::shared_ptr<Packet>)>&& packet_received);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void start();
    void stop();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);
    void set_packet_received(std::function<void(std::shared_ptr<Packet>)>&& packet_received) { _packet_received = std::move(packet_received); }

public:
    const uint32_t id;
    std::shared_ptr<Player> player {Player::create()};

private:
    Connection _connection;
    std::function<void(std::shared_ptr<Client>)> _disconnected;
    std::function<void(std::shared_ptr<Packet>)> _packet_received;
    std::atomic<bool> _is_running {false};
};
}
