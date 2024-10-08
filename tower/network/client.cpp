#include <spdlog/spdlog.h>
#include <tower/network/client.hpp>
#include <tower/network/packet/packet_base.hpp>

namespace tower::network {
using namespace tower::network::packet;

Client::Client(boost::asio::any_io_executor& executor, tcp::socket&& socket, const uint32_t client_id,
    std::function<void(std::shared_ptr<Client>&&, std::vector<uint8_t>&&)>&& packet_received,
    std::function<void(std::shared_ptr<Client>&&)>&& disconnected)
    : client_id {client_id}, _executor {executor},
    _connection {
        make_strand(executor), std::move(socket),
        [this](std::vector<uint8_t>&& buffer) {
            if (is_authenticated) {
                _heart_beater->beat();
            }

            _packet_received(shared_from_this(), std::move(buffer));
        },
        [this] { _disconnected(shared_from_this()); }
    },
    _packet_received {std::move(packet_received)},
    _disconnected {std::move(disconnected)} {
    _heart_beater = std::make_unique<HeartBeater>(executor, *this);
}

Client::~Client() {
    stop();
}

void Client::start() {
    if (_is_running.exchange(true)) return;

    _connection.open(_executor);
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

Client::HeartBeater::HeartBeater(boost::asio::any_io_executor& executor, Client& client)
    : _client {client}, _timer {std::make_shared<Timer>(executor, BEAT_INTERVAL)} {
    _on_beat = _timer->timeout.connect([this] {
        if (_dead_beats >= MAX_DEAD_BEATS) {
            spdlog::info("[Client] ({}) is not heart-beating. Disconnecting...", _client.client_id);
            _timer->stop();
            _client.stop();
            return;
        }

        if (steady_clock::now() <= _last_beat.load() + BEAT_INTERVAL) return;

        ++_dead_beats;

        flatbuffers::FlatBufferBuilder builder {64};
        const auto beat = CreateHeartBeat(builder);
        builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::HeartBeat, beat.Union()));
        _client.send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
    });
}

void Client::HeartBeater::start() {
    _last_beat = steady_clock::now();
    _timer->start();
}

void Client::HeartBeater::stop() {
    _on_beat.disconnect();
    _timer->stop();
}

void Client::HeartBeater::beat() {
    _last_beat = steady_clock::now();
    _dead_beats = 0;
}
}
