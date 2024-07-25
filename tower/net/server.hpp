#pragma once

#include <tower/container/concurrent_map.hpp>
#include <tower/net/client.hpp>
#include <tower/net/listener.hpp>
#include <tower/net/zone.hpp>
#include <tower/net/packet/packet_base.hpp>

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

    void handle_packet(std::shared_ptr<Packet> packet);
    void handle_client_join(std::shared_ptr<Client> client, const packet::ClientJoin* client_join);

    boost::asio::io_context _ctx {};
    Listener _listener {
        _ctx, 30000,
        [this](tcp::socket&& socket) { add_client(std::move(socket)); }
    };
    std::atomic<bool> _is_running {false};

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::unordered_map<uint32_t, std::weak_ptr<Zone>> _client_current_zones {};
    std::shared_ptr<Zone> _zone1 {std::make_shared<Zone>()};
    std::shared_ptr<Zone> _zone2 {std::make_shared<Zone>()};

    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;
};
}
