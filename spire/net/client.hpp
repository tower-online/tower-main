#pragma once

#include <spire/containers/concurrent_queue.hpp>
#include <spire/game/player.hpp>
#include <spire/net/connection.hpp>
#include <spire/net/packet.hpp>
#include <spire/net/packet/packet_base.hpp>

namespace spire::net {
using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id,
        ConcurrentQueue<Packet>& receive_queue);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void start();
    void stop();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);

private:
    void on_disconnection();

public:
    const uint32_t id;
    std::shared_ptr<game::Player> player;

private:
    Connection _connection;
    ConcurrentQueue<Packet>& _receive_queue;
    std::atomic<bool> _is_running {false};
};
}
