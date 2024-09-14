#pragma once

#include <tower/network/client.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/network/server_shared_state.hpp>
#include <tower/network/zone.hpp>

namespace tower::network {
using namespace tower::network::packet;
using namespace tower::world;

class Server {
    struct ClientEntry;

public:
    Server(boost::asio::any_io_executor&& executor, const std::shared_ptr<ServerSharedState>& st);
    ~Server();

    void init();
    void start();
    void stop();

private:
    boost::asio::awaitable<void> handle_packet(std::shared_ptr<Packet>&& packet);
    boost::asio::awaitable<void> handle_client_join_request(
        std::shared_ptr<Client>&& client, const ClientJoinRequest* request);
    void handle_player_enter_zone_request(std::shared_ptr<Client>&& client, const PlayerEnterZoneRequest* request);

    // Network
    std::atomic<bool> _is_running {false};
    std::unique_ptr<tcp::acceptor> _acceptor;
    boost::asio::any_io_executor _executor;
    boost::asio::strand<boost::asio::any_io_executor> _strand;
    std::shared_ptr<ServerSharedState> _st;

    std::unordered_map<uint32_t, std::unique_ptr<ClientEntry>> _client_entries {};
    std::unordered_map<uint32_t, std::shared_ptr<Zone>> _zones {};
};

struct Server::ClientEntry {
    ClientEntry(const std::shared_ptr<Client>& c, boost::signals2::connection&& on_disconnected) :
        client {c}, _on_disconnected {std::move(on_disconnected)} {}

    std::shared_ptr<Client> client;
    uint32_t current_zone_id {};

private:
    boost::signals2::connection _on_disconnected;
};
}
