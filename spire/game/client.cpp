#include <spire/game/client.hpp>
#include <spire/net/packet/packet.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace spire::game {
Client::Client(boost::asio::io_context& ctx, tcp::socket&& socket, uint32_t id)
    : _ctx(ctx), _connection(_ctx, std::move(socket), _receive_queue, [this] { on_disconnection(); }),
    _id(id) {}

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

    using namespace net::packet;
    flatbuffers::FlatBufferBuilder builder {128};
    const Vector2 position {27, 42};
    const auto entity_transform_update = CreateEntityTransformUpdate(builder, _id, &position);
    builder.FinishSizePrefixed(CreatePacket(builder, PacketType::EntityTransformUpdate, entity_transform_update.Union()));
    _connection.send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
}

void Client::stop() {
    if (!_is_running.exchange(false)) return;

    _connection.disconnect();
}

void Client::handle_packet(std::vector<uint8_t>&& buffer) {
    // const auto packet = packet::GetPacket(buffer.data());
    // if (!packet) {
    //     godot::UtilityFunctions::print("[Connection] Invalid packet");
    //     disconnect();
    //     return;
    // }
    //
    // switch (packet->packet_type()) {
    // case packet::PacketType::EntityTransformUpdate:
    //     godot::UtilityFunctions::print("[Connection] Got EntityTransformUpdate");
    // }
}

void Client::on_disconnection() {
    stop();
}
}
