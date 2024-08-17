#pragma once

#include <boost/asio.hpp>
#include <tower/system/event.hpp>

#include <chrono>

#include "spdlog/spdlog.h"

namespace tower {
using namespace std::chrono;

class Timer {
public:
    explicit Timer(boost::asio::io_context& ctx, milliseconds duration, bool autostart = false, bool one_shot = false);

    void start();
    void stop();

    Event<> timeout;

private:
    milliseconds _duration;
    bool _one_shot;

    boost::asio::io_context& _ctx;
    std::atomic<bool> _is_running{false};
};

inline Timer::Timer(boost::asio::io_context& ctx, milliseconds duration, bool autostart, bool one_shot)
    : _ctx{ctx}, _one_shot{one_shot}, _duration{duration} {
    if (autostart) start();
}

inline void Timer::start() {
    _is_running = true;

    co_spawn(_ctx, [this]()-> boost::asio::awaitable<void> {
        boost::asio::steady_timer timer{_ctx};

        do {
            timer.expires_after(_duration);
            if (auto [e] = co_await timer.async_wait(as_tuple(boost::asio::use_awaitable)); e || !_is_running) {
                break;
            }

            timeout.notify();
        } while (!_one_shot && _is_running);
    }, boost::asio::detached);
}

inline void Timer::stop() {
    _is_running = false;
}
}
