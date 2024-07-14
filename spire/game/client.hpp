#pragma once

#include <spire/containers/concurrent_queue.hpp>
#include <spire/game/player.hpp>
#include <spire/net/connection.hpp>
#include <spire/net/packet/packet.hpp>

namespace spire::game {
using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id);

    void start();
    void stop();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> packet);

private:
    void on_disconnection();

    void handle_packet(std::vector<uint8_t>&& buffer);
    void handle_packet_client_join(const net::packet::ClientJoin* client_join);

public:
    const uint32_t id;
    std::shared_ptr<Player> player;

private:
    boost::asio::io_context& _ctx;
    net::Connection _connection;

    std::atomic<bool> _is_running {false};
    ConcurrentQueue<std::vector<uint8_t>> _receive_queue;
    std::thread _worker_thread;
};
}