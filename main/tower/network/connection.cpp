#include <spdlog/spdlog.h>
#include <tower/network/connection.hpp>

namespace tower::network {
Connection::Connection(boost::asio::io_context& ctx, tcp::socket&& socket,
    std::function<boost::asio::awaitable<void>(std::vector<uint8_t>&&)>&& packet_received,
    std::function<void()>&& disconnected)
    : _ctx {ctx}, _socket {std::move(socket)}, _is_connected {_socket.is_open()},
    _packet_received {std::move(packet_received)}, _disconnected {std::move(disconnected)},
    _send_strand {make_strand(_ctx)} {}

Connection::~Connection() {
    disconnect();
}

void Connection::open() {
    // Start receiving messages
    co_spawn(_ctx, [this]()->boost::asio::awaitable<void> {
        while (_is_connected) {
            co_await receive_packet();
        }
    }, boost::asio::detached);
}

boost::asio::awaitable<bool> Connection::connect(const tcp::endpoint& remote_endpoint) {
    if (_is_connected) {
        spdlog::error("[Connection] Socket is already connected");
        co_return false;
    }

    if (const auto [ec] = co_await _socket.async_connect(remote_endpoint, as_tuple(boost::asio::use_awaitable)); ec) {
        spdlog::error("[Connection] Error connecting to {}:{}",
            remote_endpoint.address().to_string(), remote_endpoint.port());
        co_return false;
    }

    _is_connected = true;
    co_return true;
}

void Connection::disconnect() {
    if (!_is_connected.exchange(false)) return;

    boost::system::error_code ec;
    _socket.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
        spdlog::error("[Connection] Error shutting down socket: {}", ec.message());
    }

    _socket.close(ec);
    if (ec) {
        spdlog::error("[Connection] Error closing socket: {}", ec.message());
    }

    _disconnected();
}

void Connection::send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer) {
    if (!buffer || !_is_connected) return;

    co_spawn(_send_strand, [this, buffer = std::move(buffer)]()->boost::asio::awaitable<void> {
        if (!_is_connected) co_return;

        const auto [ec, _] = co_await _socket.async_send(
            boost::asio::buffer(buffer->data(), buffer->size()), as_tuple(boost::asio::use_awaitable));
        if (ec) {
            spdlog::error("[Connection] Error sending packet: {}", ec.what());
            disconnect();
            co_return;
        }
    }, boost::asio::detached);
}

boost::asio::awaitable<void> Connection::receive_packet() {
    constexpr size_t PACKET_SIZE_PREFIX_BYTES = sizeof(flatbuffers::uoffset_t);

    std::array<uint8_t, PACKET_SIZE_PREFIX_BYTES> header_buffer {};
    if (const auto [ec, _] = co_await async_read(_socket,
        boost::asio::buffer(header_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    const auto length = flatbuffers::GetSizePrefixedBufferLength(header_buffer.data());
    //TODO: Optional limit rather than hard limit?
    if (length > 65536) {
        disconnect();
        co_return;
    }
    std::vector<uint8_t> body_buffer(length - PACKET_SIZE_PREFIX_BYTES);

    if (const auto [ec, _] = co_await async_read(_socket,
        boost::asio::buffer(body_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    co_await _packet_received(std::move(body_buffer));
}
}
