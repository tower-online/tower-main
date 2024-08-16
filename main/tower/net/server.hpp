#pragma once

#include <boost/redis.hpp>
#include <tower/net/client.hpp>
#include <tower/net/listener.hpp>
#include <tower/net/packet.hpp>
#include <tower/net/zone.hpp>
#include <tower/net/packet/packet_base.hpp>
#include <tower/system/settings.hpp>
#include <tower/system/container/concurrent_map.hpp>

namespace tower::net {
using namespace tower::net::packet;
using namespace tower::world;
namespace redis = boost::redis;

class Server {
public:
    Server();
    ~Server();

    void start();
    void stop();

private:
    void add_client(tcp::socket&& socket);
    void remove_client(std::shared_ptr<Client>&& client);

    void handle_packet(std::unique_ptr<Packet> packet);
    void handle_client_join_request(std::shared_ptr<Client>&& client, const ClientJoinRequest* request);

    // Network
    boost::asio::io_context _ctx {};
    Listener _listener {
        _ctx, Settings::main_listen_port(),
        [this](tcp::socket&& socket) { add_client(std::move(socket)); }
    };
    std::atomic<bool> _is_running {false};
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::shared_ptr<EventListener<std::shared_ptr<Client>>> _on_client_disconnected;

    redis::connection _redis_connection {_ctx};

    // World
    std::unordered_map<uint32_t, std::unique_ptr<Zone>> _zones {};
    std::unordered_map<uint32_t, std::atomic<uint32_t>> _clients_current_zone {};

    // Jobs
    ConcurrentQueue<std::function<void()>> _jobs {};
    std::thread _worker_thread;
};

static std::string_view platform_to_string(ClientPlatform platform, bool lower = false);
}
