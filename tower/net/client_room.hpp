#pragma once

#include <tower/net/client.hpp>
#include <tower/system/event.hpp>

namespace tower::net {
class ClientRoom {
public:
    ClientRoom(ConcurrentQueue<std::function<void()>>& jobs,
        std::function<void(std::shared_ptr<Packet>&&)>&& packet_handler);
    ~ClientRoom();

    void start();
    void stop();

    void add_client(const std::shared_ptr<Client>& client);
    void add_client_deferred(std::shared_ptr<Client> client);
    void remove_client(uint32_t client_id);
    void remove_client_deferred(uint32_t client_id);

    void broadcast_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer, uint32_t except = 0);

private:
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients {};
    std::function<void(std::shared_ptr<Packet>&&)> _packet_handler;
    ConcurrentQueue<std::function<void()>>& _jobs;
    std::atomic<bool> _is_running {false};
};

inline ClientRoom::ClientRoom(ConcurrentQueue<std::function<void()>>& jobs,
    std::function<void(std::shared_ptr<Packet>&&)>&& packet_handler)
    : _packet_handler {std::move(packet_handler)}, _jobs {jobs} {}

inline ClientRoom::~ClientRoom() {
    stop();
}

inline void ClientRoom::start() {
    if (_is_running.exchange(true)) return;
}

inline void ClientRoom::stop() {
    if (!_is_running.exchange(false)) return;
}

inline void ClientRoom::add_client(const std::shared_ptr<Client>& client) {
    client->set_packet_received([this](std::shared_ptr<Packet> packet) {
        _jobs.push([this, packet = std::move(packet)]() mutable {
            _packet_handler(std::move(packet));
        });
    });

    _clients.insert_or_assign(client->id, client);
}

inline void ClientRoom::add_client_deferred(std::shared_ptr<Client> client) {
    _jobs.push([this, new_client = std::move(client)] {
        new_client->set_packet_received([this](std::shared_ptr<Packet> packet) {
            _jobs.push([this, packet = std::move(packet)]() mutable {
                _packet_handler(std::move(packet));
            });
        });

        _clients.insert_or_assign(new_client->id, new_client);
    });
}

inline void ClientRoom::remove_client(const uint32_t client_id) {
    if (!_clients.contains(client_id)) return;
    _clients.erase(client_id);
}

inline void ClientRoom::remove_client_deferred(const uint32_t client_id) {
    _jobs.push([this, client_id] {
        if (!_clients.contains(client_id)) return;
        _clients.erase(client_id);
    });
}

inline void ClientRoom::broadcast_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer, const uint32_t except) {
    for (auto& [id, client] : _clients) {
        if (id == except) continue;
        client->send_packet(buffer);
    }
}
}
