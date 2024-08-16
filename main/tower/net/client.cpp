#include <tower/net/client.hpp>

namespace tower::net {
Client::Client(boost::asio::io_context& ctx, tcp::socket&& socket, const uint32_t id,
    std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)>&& packet_received)
    : id(id),
    _connection(ctx, std::move(socket),
        [this](std::vector<uint8_t>&& buffer) {
            _heart_beater->beat();
            _packet_received(shared_from_this(), std::move(buffer));
        },
        [this] { disconnected.notify(shared_from_this()); }),
    _packet_received(std::move(packet_received)) {
    _heart_beater = std::make_unique<HeartBeater>(ctx, [this] {
        spdlog::info("[Client] ({}) is not heart-beating. Disconnecting...");
        stop();
    });
}

void Client::start() {
    if (_is_running.exchange(true)) return;

    _connection.open();
}

void Client::stop() {
    if (!_is_running.exchange(false)) return;

    _connection.disconnect();
}

void Client::send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer) {
    _connection.send_packet(std::move(buffer));
}

Client::HeartBeater::HeartBeater(boost::asio::io_context& ctx, std::function<void()>&& on_dead)
    : _timer{ctx, BEAT_INTERVAL}, _on_dead{std::move(on_dead)} {
    _on_beat = std::make_shared<EventListener<>>([this] {
        if (_dead_beats > MAX_DEAD_BEATS) {
            _timer.stop();
            _on_dead();
        }

        if (steady_clock::now() > _last_beat.load() + BEAT_INTERVAL) {
            ++_dead_beats;

            //TODO: Send HeartBeat
        }
    });

    _timer.timeout.subscribe(_on_beat->shared_from_this());
}

    void Client::HeartBeater::start() {
    _last_beat = steady_clock::now();
    _timer.start();
}

    void Client::HeartBeater::stop() {
    _timer.stop();
}

    void Client::HeartBeater::beat() {
    _last_beat = steady_clock::now();
    _dead_beats = 0;
}
}