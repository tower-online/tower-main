#pragma once

#include <tower/network/client.hpp>
#include <tower/network/packet.hpp>
#include <tower/network/packet/packet_base.hpp>
#include <tower/network/server_shared_state.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/profiler.hpp>
#include <tower/network/login_handler.hpp>

namespace tower::network {
using namespace tower::game;
using namespace tower::network::packet;
using namespace tower::world;

class Server {
public:
    explicit Server(const std::shared_ptr<ServerSharedState>& shared_state);
    ~Server();

    void init();
    void start();
    void stop();

private:
    void add_client_deferred(tcp::socket&& socket);
    void remove_client_deferred(std::shared_ptr<Client> client);

    void broadcast(std::shared_ptr<flatbuffers::DetachedBuffer>&& buffer);

    void handle_packet(std::unique_ptr<Packet> packet);
    void handle_client_join_request(std::unique_ptr<Packet> packet, const ClientJoinRequest* request);
    void handle_player_enter_zone_request(std::shared_ptr<Client>&& client, const PlayerEnterZoneRequest* request);
    void handle_player_join_party_request(std::shared_ptr<Client>&& client, const PlayerJoinPartyRequest* request);
    void handle_player_join_party_response(std::shared_ptr<Client>&& client, const PlayerJoinPartyResponse* response);
    void handle_player_leave_party(std::shared_ptr<Client>&& client, const PlayerLeaveParty* response);
    void handle_player_chat(std::shared_ptr<Client>&& client, const PlayerChat* chat, size_t buffer_size);

    std::atomic<bool> _is_running {false};
    std::unique_ptr<tcp::acceptor> _acceptor;
    std::shared_ptr<ServerSharedState> _shared_state;

    LoginHandler _login_handler;
    Profiler _profiler {};

    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::unordered_map<uint32_t, uint32_t> _clients_current_zone {};
    std::unordered_map<uint32_t, std::unique_ptr<Zone>> _zones {};
};
}
