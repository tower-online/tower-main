#pragma once

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <spdlog/spdlog.h>

namespace spire::net {
    using boost::asio::ip::tcp;

    class Listener final : boost::noncopyable {
    public:
        Listener(boost::asio::io_context& ctx, uint16_t port,
            std::function<void(boost::asio::io_context&, tcp::socket&&)>&& on_acceptance);
        ~Listener() = default;

        void start();
        void stop();

    private:
        boost::asio::awaitable<void> listen();

        boost::asio::io_context& ctx_;
        tcp::acceptor acceptor_;
        std::function<void(boost::asio::io_context&, tcp::socket&&)> on_acceptance_;

        std::atomic<bool> listening_ {false};
    };

    inline Listener::Listener(boost::asio::io_context& ctx, const uint16_t port,
        std::function<void(boost::asio::io_context&, tcp::socket&&)>&& on_acceptance)
        : ctx_(ctx), acceptor_(ctx, tcp::endpoint(tcp::v4(), port)),
        on_acceptance_(std::move(on_acceptance)) {}

    inline void Listener::start() {
        listening_ = true;
        co_spawn(ctx_, listen(), boost::asio::detached);
    }

    inline void Listener::stop() {
        if (!listening_.exchange(false)) return;

        try {
            acceptor_.close();
        } catch (const boost::system::system_error& e) {
            spdlog::error("[Listener] Error closing: {}", e.what());
        }
    }

    inline boost::asio::awaitable<void> Listener::listen() {
        spdlog::info("[Listener] Starts accepting on {}:{}", acceptor_.local_endpoint().address().to_string(),
            acceptor_.local_endpoint().port());

        while (listening_) {
            tcp::socket socket {ctx_};

            if (const auto [ec] = co_await acceptor_.async_accept(socket, as_tuple(boost::asio::use_awaitable)); ec) {
                spdlog::error("[Listener] Error on accepting: {}", ec.what());
                continue;
            }

            spdlog::info("[Listener] Accepting from {}:{}", socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());
            on_acceptance_(ctx_, std::move(socket));
        }
    }
}