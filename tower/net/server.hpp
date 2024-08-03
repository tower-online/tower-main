#pragma once

#include <tower/container/concurrent_map.hpp>
#include <tower/net/client.hpp>
#include <tower/net/listener.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/world/zone.hpp>

#include <chrono>

using namespace std::chrono;

namespace tower::net {
class Server {
    constexpr static auto TICK_INTERVAL = 100ms;

public:
    Server();
    ~Server();

    void start();
    void stop();

private:
    void add_client(tcp::socket&& socket);
    void remove_client(std::shared_ptr<Client> client);

    void handle_packet(std::shared_ptr<Packet> packet);
    void handle_client_join(std::shared_ptr<Client> client, const ClientJoin* client_join);

    // Network
    boost::asio::io_context _ctx {};
    Listener _listener {
        _ctx, 30000,
        [this](tcp::socket&& socket) { add_client(std::move(socket)); }
    };
    std::atomic<bool> _is_running {false};
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::shared_ptr<EventListener<std::shared_ptr<Client>>> _on_client_disconnected;

    // World
    Zone _default_zone {0, "test_zone"};

    // Jobs
    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;
};
}