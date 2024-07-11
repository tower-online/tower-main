#pragma once

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <flatbuffers/flatbuffers.h>
#include <spire/containers/concurrent_queue.hpp>
#include <spdlog/spdlog.h>

namespace spire::net {
using boost::asio::ip::tcp;

class Connection final : boost::noncopyable {
public:
    Connection(boost::asio::io_context& ctx, tcp::socket&& socket,
        std::function<void(std::vector<uint8_t>&&)>&& message_handler);
    ~Connection();

    void run();
    boost::asio::awaitable<bool> connect(const tcp::endpoint& remote_endpoint);
    void disconnect();
    void send_message(std::shared_ptr<flatbuffers::DetachedBuffer> message);
    void set_no_delay(bool delay);

    bool is_connected() const { return is_connected_; }
    tcp::endpoint local_endpoint() const { return socket_.local_endpoint(); }
    tcp::endpoint remote_endpoint() const { return socket_.remote_endpoint(); }

private:
    boost::asio::awaitable<void> receive_message();

    boost::asio::io_context& ctx_;
    tcp::socket socket_;
    std::atomic<bool> is_connected_;

    ConcurrentQueue<std::vector<uint8_t>> receive_queue_;
    ConcurrentQueue<std::shared_ptr<flatbuffers::DetachedBuffer>> send_queue_ {};
    std::atomic<bool> is_sending_ {false};
    std::thread worker_;
    std::function<void(std::vector<uint8_t>&&)> message_handler_;
};


inline Connection::Connection(boost::asio::io_context& ctx, tcp::socket&& socket,
    std::function<void(std::vector<uint8_t>&&)>&& message_handler)
    : ctx_(ctx), socket_(std::move(socket)), is_connected_(socket_.is_open()), message_handler_(std::move(message_handler)) {}

inline Connection::~Connection() {
    disconnect();

    if (worker_.joinable()) worker_.join();
}

inline void Connection::run() {
    // Start receiving messages
    co_spawn(ctx_, [this]()->boost::asio::awaitable<void> {
        while (is_connected_) {
            co_await receive_message();
        }
    }, boost::asio::detached);

    // Start handling messages
    worker_ = std::thread([this] {
        while (is_connected_) {
            std::vector<uint8_t> message;
            //TODO: Remove busy-waiting
            if (!receive_queue_.try_pop(message)) continue;
            message_handler_(std::move(message));
        }
    });
    worker_.detach();
}

inline boost::asio::awaitable<bool> Connection::connect(const tcp::endpoint& remote_endpoint) {
    if (is_connected_) {
        spdlog::error("[PeerTCP] Socket is already connected");
        co_return false;
    }

    const auto [ec] = co_await socket_.async_connect(remote_endpoint, as_tuple(boost::asio::use_awaitable));
    if (ec) {
        spdlog::error("[PeerTCP] Error connecting to {}:{}", remote_endpoint.address().to_string(),
            remote_endpoint.port());
        co_return false;
    }

    is_connected_ = true;
    co_return true;
}

inline void Connection::disconnect() {
    if (!is_connected_.exchange(false)) return;

    receive_queue_.clear();

    try {
        socket_.shutdown(tcp::socket::shutdown_both);

        //TODO: Process remaining send queue?
        socket_.close();
    } catch (const boost::system::system_error& e) {
        spdlog::error("[PeerTCP] Error shutting down socket: {}", e.what());
    }
}

inline void Connection::send_message(std::shared_ptr<flatbuffers::DetachedBuffer> message) {
    if (!message || !is_connected_) return;

    send_queue_.push(std::move(message));

    // Send all messages in send_queue_
    if (is_sending_.exchange(true)) return;
    co_spawn(ctx_, [this]()->boost::asio::awaitable<void> {
        while (true) {
            std::vector<uint8_t> message;
            if (!receive_queue_.try_pop(message)) break;

            if (auto [ec, _] = co_await socket_.async_send(boost::asio::buffer(message.data(), message.size()),
                as_tuple(boost::asio::use_awaitable)); ec) {
                spdlog::error("[PeerTCP] Error sending message: {}", ec.what());
                disconnect();
                co_return;
            }
        }

        is_sending_ = false;
    }, boost::asio::detached);
}

inline void Connection::set_no_delay(const bool delay) {
    try {
        socket_.set_option(tcp::no_delay(delay));
    } catch (const boost::system::system_error& e) {
        spdlog::error("[PeerTCP] Error setting no-delay: {}", e.what());
    }
}

inline boost::asio::awaitable<void> Connection::receive_message() {
    constexpr size_t MESSAGE_SIZE_PREFIX_BYTES = sizeof(flatbuffers::uoffset_t);

    std::array<uint8_t, MESSAGE_SIZE_PREFIX_BYTES> header_buffer {};
    if (const auto [ec, _] = co_await async_read(socket_,
        boost::asio::buffer(header_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    const auto length = flatbuffers::GetSizePrefixedBufferLength(header_buffer.data());
    std::vector<uint8_t> body_buffer(length - MESSAGE_SIZE_PREFIX_BYTES);

    if (const auto [ec, _] = co_await async_read(socket_,
        boost::asio::buffer(body_buffer),
        as_tuple(boost::asio::use_awaitable)); ec) {
        disconnect();
        co_return;
    }

    receive_queue_.push(std::move(body_buffer));
}
}