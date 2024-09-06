#pragma once

#include <boost/redis.hpp>
#include <tower/network/client.hpp>
#include <tower/network/listener.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/system/settings.hpp>

namespace tower::network {
namespace redis = boost::redis;
using namespace tower::network::packet;
using namespace tower::world;

class Server {
public:
    explicit Server(size_t num_workers);
    ~Server();

    void init();
    void start();
    void stop();

private:
    void add_client_deferred(tcp::socket&& socket);
    void remove_client_deferred(std::shared_ptr<Client>&& client);

    void handle_packet(std::unique_ptr<Packet> packet);
    boost::asio::awaitable<void> handle_packet_internal(std::shared_ptr<Packet>&& packet);
    boost::asio::awaitable<void> handle_client_join_request(std::shared_ptr<Client>&& client,
        const ClientJoinRequest* request);

    // Network
    std::atomic<bool> _is_running {false};
    boost::asio::io_context _ctx {};
    boost::asio::thread_pool _workers;
    const size_t _num_workers;
    boost::asio::strand<boost::asio::io_context::executor_type> _jobs_strand {make_strand(_ctx)};

    Listener _listener {
        _ctx, Settings::main_listen_port(),
        [this](tcp::socket&& socket) { add_client_deferred(std::move(socket)); },
        Settings::main_listen_backlog()
    };
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::unordered_map<uint32_t, signals::connection> _clients_on_disconnected {};

    redis::connection _redis_connection {_ctx};

    std::unordered_map<uint32_t, std::unique_ptr<Zone>> _zones {};
    std::unordered_map<uint32_t, std::atomic<uint32_t>> _clients_current_zone {};

};

static std::string_view platform_to_string(ClientPlatform platform, bool lower = false);
}
