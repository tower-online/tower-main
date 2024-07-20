#pragma once

#include <spire/game/player.hpp>
#include <spire/net/connection.hpp>
#include <spire/net/packet.hpp>

namespace spire::net {
using boost::asio::ip::tcp;

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
    std::shared_ptr<game::Player> player;

private:
    Connection _connection;
    std::function<void(std::shared_ptr<Client>)> _disconnected;
    std::function<void(std::shared_ptr<Packet>)> _packet_received;
    std::atomic<bool> _is_running {false};
};
}
