#pragma once

#include <tower/network/connection.hpp>
#include <tower/player/player.hpp>
#include <tower/system/timer.hpp>

namespace tower::network {
using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
    class HeartBeater;

public:
    Client(boost::asio::any_io_executor& executor, tcp::socket&& socket, uint32_t entry_id,
        std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)>&& packet_received);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void start();
    void stop();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);

    bool is_running() const { return _is_running; }

public:
    const uint32_t entry_id;
    bool is_authenticated {false};
    boost::signals2::signal<void(const std::shared_ptr<Client>&)> disconnected;
    std::shared_ptr<player::Player> player;

private:
    std::atomic<bool> _is_running {false};
    boost::asio::any_io_executor& _executor;

    Connection _connection;
    const std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)> _packet_received;
    std::unique_ptr<HeartBeater> _heart_beater;
};

class Client::HeartBeater {
    static constexpr uint32_t MAX_DEAD_BEATS = 3;
    static constexpr seconds BEAT_INTERVAL = 5s;

public:
    HeartBeater(boost::asio::any_io_executor& executor, Client& client);

    void start();
    void stop();
    void beat();

private:
    Client& _client;
    std::atomic<steady_clock::time_point> _last_beat;
    std::atomic<uint32_t> _dead_beats {0};

    std::shared_ptr<Timer> _timer;
    boost::signals2::connection _on_beat;
};
}
