#pragma once

#include <boost/redis.hpp>
#include <boost/thread.hpp>
#include <boost/thread/detail/thread_group.hpp>
#include <tower/network/client.hpp>
#include <tower/network/listener.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/system/settings.hpp>

namespace tower::network {
using namespace tower::network::packet;
using namespace tower::world;
namespace redis = boost::redis;

class Server {
public:
    Server(size_t num_io_threads, size_t num_worker_threads);
    ~Server();

    void init();
    void start();
    void stop();
    void join();

private:
    void add_client_deferred(tcp::socket&& socket);
    void remove_client_deferred(std::shared_ptr<Client>&& client);

    void handle_packet(std::unique_ptr<Packet> packet);
    void handle_client_join_request(std::shared_ptr<Client>&& client, const ClientJoinRequest* request);

    // Network
    std::atomic<bool> _is_running {false};
    boost::asio::io_context _ctx {};
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> _io_guard;
    boost::thread_group _io_threads {};
    size_t _num_io_threads;

    Listener _listener {
        _ctx, Settings::main_listen_port(),
        [this](tcp::socket&& socket) { add_client_deferred(std::move(socket)); },
        Settings::main_listen_backlog()
    };
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::unordered_map<uint32_t, signals::connection> _clients_on_disconnected {};

    redis::connection _redis_connection {_ctx};

    // World
    std::unordered_map<uint32_t, std::unique_ptr<Zone>> _zones {};
    std::unordered_map<uint32_t, std::atomic<uint32_t>> _clients_current_zone {};

    // Jobs
    ConcurrentQueue<std::function<void()>> _jobs {};
    boost::asio::thread_pool _worker_threads;
};

static std::string_view platform_to_string(ClientPlatform platform, bool lower = false);
}
