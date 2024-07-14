#include <spdlog/spdlog.h>
#include <spire/game/client.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace spire::game {
Client::Client(boost::asio::io_context& ctx, tcp::socket&& socket, const uint32_t id)
    : _ctx(ctx), _connection(_ctx, std::move(socket), _receive_queue, [this] { on_disconnection(); }),
    id(id), player(std::make_shared<Player>()) {}

void Client::start() {
    if (_is_running.exchange(true)) return;

    _connection.open();

    // Start handling messages
    _worker_thread = std::thread([this] {
        while (_is_running) {
            std::vector<uint8_t> message;
            if (!_receive_queue.try_pop(message)) {
                std::this_thread::sleep_for(10ms);
                continue;
            }

            handle_packet(std::move(message));
        }
    });
    _worker_thread.detach();
}

void Client::stop() {
    if (!_is_running.exchange(false)) return;

    _connection.disconnect();
}

void Client::send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> packet) {
    _connection.send_packet(std::move(packet));
}

void Client::handle_packet(std::vector<uint8_t>&& buffer) {
    using namespace net::packet;

    const auto packet = GetPacket(buffer.data());
    if (!packet) {
        spdlog::warn("[Client] Invalid packet");
        // disconnect();
        return;
    }

    switch (packet->packet_type()) {
    case PacketType::ClientJoin:
        handle_packet_client_join(packet->packet_as<ClientJoin>());
        break;

    default:
        break;
    }
}

void Client::handle_packet_client_join(const net::packet::ClientJoin* client_join) {
    spdlog::info("[Client] ({}) responsed to ClientJoin", id);
}

void Client::on_disconnection() {
    stop();
}
}
