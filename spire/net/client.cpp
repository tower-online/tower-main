#include <spdlog/spdlog.h>
#include <spire/net/client.hpp>

namespace spire::net {
Client::Client(boost::asio::io_context& ctx, tcp::socket&& socket, const uint32_t id,
    ConcurrentQueue<Packet>& receive_queue)
    : id(id), player(std::make_shared<game::Player>()),
    _connection(ctx, std::move(socket),
        [this](std::vector<uint8_t>&& buffer) {
            _receive_queue.push(Packet {shared_from_this(), std::move(buffer)});
        },
        [this] { on_disconnection(); }),
    _receive_queue(receive_queue) {}

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

void Client::on_disconnection() {
    stop();

    //TODO: Remove from server
}
}
