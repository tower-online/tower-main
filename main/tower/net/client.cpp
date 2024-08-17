#include <tower/net/client.hpp>
#include <tower/net/packet/packet_base.hpp>

namespace tower::net {
using namespace tower::net::packet;

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
    _heart_beater = std::make_unique<HeartBeater>(ctx, *this);
}

Client::~Client() {
    stop();
}

void Client::start() {
    if (_is_running.exchange(true)) return;

    _connection.open();
    _heart_beater->start();
}

void Client::stop() {
    if (!_is_running.exchange(false)) return;

    _heart_beater->stop();
    _connection.disconnect();
}

void Client::send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer) {
    _connection.send_packet(std::move(buffer));
}

Client::HeartBeater::HeartBeater(boost::asio::io_context& ctx, Client& client)
    : _client {client}, _timer {ctx, BEAT_INTERVAL} {
    _on_beat = std::make_shared<EventListener<>>([this] {
        if (_dead_beats >= MAX_DEAD_BEATS) {
            spdlog::info("[Client] ({}) is not heart-beating. Disconnecting...", _client.id);
            _timer.stop();
            _client.stop();
            return;
        }

        if (steady_clock::now() <= _last_beat.load() + BEAT_INTERVAL) return;

        ++_dead_beats;

        flatbuffers::FlatBufferBuilder builder {32};
        const auto beat = CreateHeartBeat(builder);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::HeartBeat, beat.Union()));
        _client.send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
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
