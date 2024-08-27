#pragma once

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <spdlog/spdlog.h>

namespace tower::network {
using boost::asio::ip::tcp;

class Listener final : boost::noncopyable {
public:
    Listener(boost::asio::io_context& ctx, uint16_t port,
        std::function<void(tcp::socket&&)>&& on_acceptance,
        int backlog = boost::asio::socket_base::max_listen_connections);
    ~Listener() = default;

    void start();
    void stop();

private:
    boost::asio::awaitable<void> listen();

    boost::asio::io_context& _ctx;
    tcp::acceptor _acceptor;
    std::function<void(tcp::socket&&)> _on_acceptance;

    std::atomic<bool> _listening {false};
};

inline Listener::Listener(boost::asio::io_context& ctx, const uint16_t port,
    std::function<void(tcp::socket&&)>&& on_acceptance,
    const int backlog)
    : _ctx(ctx), _acceptor(ctx, tcp::endpoint(tcp::v4(), port)),
    _on_acceptance(std::move(on_acceptance)) {
    _acceptor.listen(backlog);
}

inline void Listener::start() {
    _listening = true;
    co_spawn(_ctx, listen(), boost::asio::detached);
}

inline void Listener::stop() {
    if (!_listening.exchange(false)) return;

    try {
        _acceptor.close();
    } catch (const boost::system::system_error& e) {
        spdlog::error("[Listener] Error closing: {}", e.what());
    }
}

inline boost::asio::awaitable<void> Listener::listen() {
    spdlog::info("[Listener] Starts accepting on {}:{}", _acceptor.local_endpoint().address().to_string(),
        _acceptor.local_endpoint().port());

    while (_listening) {
        tcp::socket socket {_ctx};

        if (const auto [ec] = co_await _acceptor.async_accept(socket, as_tuple(boost::asio::use_awaitable)); ec) {
            spdlog::error("[Listener] Error on accepting: {}", ec.what());
            continue;
        }

        spdlog::info("[Listener] Accepting from {}:{}", socket.remote_endpoint().address().to_string(),
            socket.remote_endpoint().port());
        _on_acceptance(std::move(socket));
    }
}
}
