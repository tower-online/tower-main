#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace tower {
template <typename... Args>
class EventListener;

template <typename... Args>
class Event {
public:
    Event() = default;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    void subscribe(std::shared_ptr<EventListener<Args...>>&& listener) {
        _listeners[listener->id] = listener;
    }

    void unsubscribe(uint32_t listener_id) {
        _listeners.erase(listener_id);
    }

    void notify(Args... args) {
        for (auto& [id, listener] : _listeners) {
            if (auto l = listener.lock()) {
                l->call(args...);
            } else {
                _listeners.erase(id);
            }
        }
    }

private:
    std::unordered_map<uint32_t, std::weak_ptr<EventListener<Args...>>> _listeners {};
};

template <typename... Args>
class EventListener : public std::enable_shared_from_this<EventListener<Args...>> {
public:
    explicit EventListener(std::function<void(Args...)>&& function)
        : id {++_id_generator}, _function {std::move(function)} {}


    void call(Args... args) {
        _function(args...);
    }

    const uint32_t id;

private:
    std::function<void(Args...)> _function;

    inline static std::atomic<uint32_t> _id_generator {0};
};
}
