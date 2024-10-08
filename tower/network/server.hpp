#pragma once

#include <tower/network/client.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/network/server_shared_state.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/profiler.hpp>

namespace tower::network {
using namespace tower::network::packet;
using namespace tower::world;

class Server {
public:
    Server(boost::asio::any_io_executor&& executor, const std::shared_ptr<ServerSharedState>& st);
    ~Server();

    void init();
    void start();
    void stop();

private:
    void add_client_deferred(tcp::socket&& socket);
    void remove_client_deferred(std::shared_ptr<Client> client);

    boost::asio::awaitable<void> handle_packet(std::shared_ptr<Packet>&& packet);
    boost::asio::awaitable<void> handle_client_join_request(
        std::shared_ptr<Client>&& client, const ClientJoinRequest* request);
    void handle_player_enter_zone_request(std::shared_ptr<Client>&& client, const PlayerEnterZoneRequest* request);

    std::atomic<bool> _is_running {false};
    std::unique_ptr<tcp::acceptor> _acceptor;
    boost::asio::any_io_executor _executor;
    boost::asio::strand<boost::asio::any_io_executor> _strand;
    std::shared_ptr<ServerSharedState> _st;

    Profiler _profiler {};

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::unordered_map<uint32_t, uint32_t> _clients_current_zone {};
    std::unordered_map<uint32_t, std::unique_ptr<Zone>> _zones {};
};
}
