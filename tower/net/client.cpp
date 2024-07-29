#include <spdlog/spdlog.h>
#include <tower/net/client.hpp>

namespace tower::net {
Client::Client(boost::asio::io_context& ctx, tcp::socket&& socket, const uint32_t id,
    std::function<void(std::shared_ptr<Packet>)>&& packet_received)
    : id(id),
    _connection(ctx, std::move(socket),
        [this](std::vector<uint8_t>&& buffer) {
            _packet_received(std::make_shared<Packet>(shared_from_this(), std::move(buffer)));
        },
        [this] { disconnected.notify(shared_from_this()); }),
    _packet_received(std::move(packet_received)) {}

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
}
