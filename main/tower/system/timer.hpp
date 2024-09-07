#pragma once

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <chrono>

namespace tower {
using namespace std::chrono;

class Timer : public std::enable_shared_from_this<Timer> {
public:
    explicit Timer(boost::asio::io_context& ctx, milliseconds duration, bool autostart = false, bool one_shot = false);
    ~Timer();

    void start();
    void stop();

    boost::signals2::signal<void()> timeout {};

private:
    const milliseconds _duration;
    const bool _one_shot;

    std::atomic<bool> _is_running {false};
    boost::asio::io_context& _ctx;
    boost::asio::steady_timer _timer {_ctx};
};

inline Timer::Timer(boost::asio::io_context& ctx, const milliseconds duration, const bool autostart, const bool one_shot)
    : _duration {duration}, _one_shot {one_shot}, _ctx {ctx} {
    if (autostart) start();
}

inline Timer::~Timer() {
    stop();
}


inline void Timer::start() {
    if (_is_running.exchange(true)) return;

    // Capture "self" to ensure lifetime of Timer in detached coroutine
    co_spawn(_ctx, [self = shared_from_this()]()->boost::asio::awaitable<void> {
        do {
            self->_timer.expires_after(self->_duration);

            if (auto [ec] = co_await self->_timer.async_wait(as_tuple(boost::asio::use_awaitable));
                ec || !self->_is_running) {
                co_return;
            }

            self->timeout();
        } while (!self->_one_shot && self->_is_running);
    }, boost::asio::detached);
}

inline void Timer::stop() {
    if (!_is_running.exchange(false)) return;

    _timer.cancel();
}
}
