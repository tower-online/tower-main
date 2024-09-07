#pragma once

#include <tower/network/client.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/server_shared_state.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/packet/packet_base.hpp>

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
    void handle_packet(std::shared_ptr<Packet> packet);
    boost::asio::awaitable<void> handle_packet_internal(std::shared_ptr<Packet>&& packet);
    boost::asio::awaitable<void> handle_client_join_request(std::shared_ptr<Client>&& client,
        const ClientJoinRequest* request);

    // Network
    std::atomic<bool> _is_running {false};
    std::unique_ptr<tcp::acceptor> _acceptor;
    boost::asio::any_io_executor _executor;
    boost::asio::strand<boost::asio::any_io_executor> _strand;
    std::shared_ptr<ServerSharedState> _st;

    std::unordered_map<uint32_t, std::shared_ptr<ClientEntry>> _client_entries {};
    std::unordered_map<uint32_t, std::shared_ptr<Zone>> _zones {};
};

struct Server::ClientEntry {
    Server::ClientEntry(
        const std::shared_ptr<Client>& c, boost::signals2::connection&& on_disconnected)
        : entry_id {++_entry_id_generator}, client {c}, _on_disconnected {std::move(on_disconnected)} {
        if (!client) return;
        client->entry_id = entry_id;
    }

    const uint32_t entry_id;
    std::shared_ptr<Client> client;
    std::shared_ptr<Zone> current_zone {};

private:
    inline static std::atomic<uint32_t> _entry_id_generator {0};

    boost::signals2::connection _on_disconnected;
};

static std::string_view platform_to_string(ClientPlatform platform, bool lower = false);
}
