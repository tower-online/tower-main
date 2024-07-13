#pragma once

#include <spire/containers/concurrent_queue.hpp>
#include <spire/net/connection.hpp>

namespace spire::game {
using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id);

    void start();
    void stop();

private:
    void handle_packet(std::vector<uint8_t>&& buffer);
    void on_disconnection();

    boost::asio::io_context& _ctx;
    net::Connection _connection;
    const uint32_t _id;

    std::atomic<bool> _is_running {false};
    ConcurrentQueue<std::vector<uint8_t>> _receive_queue;
    std::thread _worker_thread;
};
}