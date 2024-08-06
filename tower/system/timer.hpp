#pragma once

#include <tower/system/event.hpp>

#include <chrono>

namespace tower {
using namespace std::chrono;

class Timer {
public:
    explicit Timer(milliseconds duration, bool one_shot = true);

    Event<> timeout;

    const bool one_shot;

    bool is_timedout() const;

private:
    time_point<steady_clock> _start_time;
    milliseconds _duration;
};

inline Timer::Timer(const milliseconds duration, const bool one_shot)
    : one_shot {one_shot}, _duration {duration} {
    _start_time = steady_clock::now();
}

inline bool Timer::is_timedout() const {
    return steady_clock::now() >= _start_time + _duration;
}
}
