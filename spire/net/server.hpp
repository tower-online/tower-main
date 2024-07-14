#pragma once

#include <spire/containers/concurrent_map.hpp>
#include <spire/net/client.hpp>
#include <spire/net/listener.hpp>
#include <spire/net/packet/packet_base.hpp>

namespace spire::net {
class Server {
public:
    Server();
    ~Server();

    void start();
    void stop();

private:
    void add_client(tcp::socket&& socket);

    void handle_packet(Packet&& packet);
    void handle_packet_client_join(std::shared_ptr<Client> client, const packet::ClientJoin* client_join);

    boost::asio::io_context _ctx {};
    Listener listener_ {
        _ctx, 30000,
        [this](tcp::socket&& socket) { add_client(std::move(socket)); }
    };
    ConcurrentMap<uint32_t, std::shared_ptr<Client>> _clients {};
    std::atomic<bool> _is_running {false};

    ConcurrentQueue<Packet> _receive_queue {};
    std::thread _worker_thread {}; //TODO: Handle global packets on server-side, handle local packets on zone-side
};
}
