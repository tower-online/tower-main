#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>

namespace tower::network {
using namespace std::chrono;

class Profiler {
    static constexpr milliseconds WINDOW_LENGTH {5000ms};

    struct PacketRecord {
        steady_clock::time_point timestamp;
        uint16_t bytes;
    };

public:
    void add_packet(uint32_t packet_bytes) {
        const auto now {steady_clock::now()};

        {
            std::lock_guard lock {_records_mutex};
            _records.emplace(now, packet_bytes);
            _total_packets += 1;
            _total_bytes += packet_bytes;
        }

        renew();
    }

    void renew() {
        if (_is_renewing.exchange(true)) return;

        static constexpr size_t max_pops {10000};
        const auto now {steady_clock::now()};
        size_t pops {0};

        std::lock_guard lock {_records_mutex};
        while (pops++ < max_pops && !_records.empty()) {
            const auto& [timestamp, bytes] {_records.front()};
            if (now < timestamp + WINDOW_LENGTH) break;

            _total_packets -= 1;
            _total_bytes -= bytes;

            _records.pop();
        }

        _is_renewing = false;
    }

    uint64_t get_pps() const {
        return _total_packets * 1000 / WINDOW_LENGTH.count();
    }

    uint64_t get_bps() const {
        return _total_bytes * 1000 / WINDOW_LENGTH.count();
    }

private:
    std::queue<PacketRecord> _records {};
    std::mutex _records_mutex {};
    std::atomic<bool> _is_renewing {false};
    std::atomic<uint64_t> _total_packets {};
    std::atomic<uint64_t> _total_bytes {};
};
}