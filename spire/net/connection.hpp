#pragma once

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <flatbuffers/flatbuffers.h>
#include <spdlog/spdlog.h>
#include <spire/containers/concurrent_queue.hpp>

#include <iostream>

namespace spire::net {
using boost::asio::ip::tcp;

class Connection final : boost::noncopyable {
public:
    Connection(boost::asio::io_context& ctx, tcp::socket&& socket,
        std::function<void(std::vector<uint8_t>&&)>&& packet_received, std::function<void()>&& disconnected);
    ~Connection();

    void open();
    boost::asio::awaitable<bool> connect(const tcp::endpoint& remote_endpoint);
    void disconnect();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);
    void set_no_delay(bool no_delay);

    bool is_connected() const { return _is_connected; }
    tcp::endpoint local_endpoint() const { return _socket.local_endpoint(); }
    tcp::endpoint remote_endpoint() const { return _socket.remote_endpoint(); }

private:
    boost::asio::awaitable<void> receive_packet();

    boost::asio::io_context& _ctx;
    tcp::socket _socket;
    std::atomic<bool> _is_connected;
    std::function<void(std::vector<uint8_t>&&)> _packet_received;
    std::function<void()> _disconnected;
    ConcurrentQueue<std::shared_ptr<flatbuffers::DetachedBuffer>> _send_queue {};
    std::atomic<bool> _is_sending {false};
};


inline Connection::Connection(boost::asio::io_context& ctx, tcp::socket&& socket,
    std::function<void(std::vector<uint8_t>&&)>&& packet_received, std::function<void()>&& disconnected)
    : _ctx(ctx), _socket(std::move(socket)), _is_connected(_socket.is_open()),
    _packet_received(std::move(packet_received)), _disconnected(std::move(disconnected)) {}

inline Connection::~Connection() {
    disconnect();
}

inline void Connection::open() {
    // Start receiving messages
    co_spawn(_ctx, [this]()->boost::asio::awaitable<void> {
        while (_is_connected) {
            co_await receive_packet();
        }
    }, boost::asio::detached);
}

inline boost::asio::awaitable<bool> Connection::connect(const tcp::endpoint& remote_endpoint) {
    if (_is_connected) {
        spdlog::error("[Connection] Socket is already connected");
        co_return false;
    }

    const auto [ec] = co_await _socket.async_connect(remote_endpoint, as_tuple(boost::asio::use_awaitable));
    if (ec) {
        spdlog::error("[Connection] Error connecting to {}:{}", remote_endpoint.address().to_string(),
            remote_endpoint.port());
        co_return false;
    }

    _is_connected = true;
    co_return true;
}

inline void Connection::disconnect() {
    if (!_is_connected.exchange(false)) return;

    try {
        _socket.shutdown(tcp::socket::shutdown_both);

        //TODO: Process remaining send queue?
        _socket.close();
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Connection] Error shutting down socket: {}", e.what());
    }

    _disconnected();
}

inline void Connection::send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer) {
    if (!buffer || !_is_connected) return;

    _send_queue.push(std::move(buffer));

    // Send all packets in send_queue_
    if (_is_sending.exchange(true)) return;
    co_spawn(_ctx, [this]()->boost::asio::awaitable<void> {
        while (!_send_queue.empty()) {
            std::shared_ptr<flatbuffers::DetachedBuffer> buffer;
            if (!_send_queue.try_pop(buffer) || !buffer) {
                spdlog::warn("[Connection] Invalid packet to send");
                break;
            }

            if (auto [ec, _] = co_await _socket.async_send(boost::asio::buffer(buffer->data(), buffer->size()),
                as_tuple(boost::asio::use_awaitable)); ec) {
                spdlog::error("[Connection] Error sending packet: {}", ec.what());
                disconnect();
                co_return;
            }
        }

        _is_sending = false;
    }, boost::asio::detached);
}

inline void Connection::set_no_delay(const bool no_delay) {
    try {
        _socket.set_option(tcp::no_delay(no_delay));
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Connection] Error setting no-delay: {}", e.what());
    }
}

inline boost::asio::awaitable<void> Connection::receive_packet() {
    constexpr size_t PACKET_SIZE_PREFIX_BYTES = sizeof(flatbuffers::uoffset_t);

    std::array<uint8_t, PACKET_SIZE_PREFIX_BYTES> header_buffer {};
    if (const auto [ec, _] = co_await async_read(_socket,
        boost::asio::buffer(header_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    const auto length = flatbuffers::GetSizePrefixedBufferLength(header_buffer.data());
    std::vector<uint8_t> body_buffer(length - PACKET_SIZE_PREFIX_BYTES);

    if (const auto [ec, _] = co_await async_read(_socket,
        boost::asio::buffer(body_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    _packet_received(std::move(body_buffer));
}
}
