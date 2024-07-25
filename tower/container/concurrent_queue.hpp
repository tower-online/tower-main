#pragma once

#include <mutex>
#include <queue>

#include <iostream>

namespace tower {
/*
 * A thread-safe queue for single consumer and mutliple producers
 */
template <typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;
    ~ConcurrentQueue() = default;
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    void push(const T& value);
    void push(T&& value);
    bool try_pop(T& target);
    bool empty() const;
    size_t size() const;
    void clear();

private:
    std::queue<T> _queue {};
    mutable std::mutex _mutex {};
};

template <typename T>
void ConcurrentQueue<T>::push(const T& value) {
    std::lock_guard lock {_mutex};

    _queue.push(value);
}

template <typename T>
void ConcurrentQueue<T>::push(T&& value) {
    std::lock_guard lock {_mutex};

    _queue.push(std::forward<T>(value));
}

template <typename T>
bool ConcurrentQueue<T>::try_pop(T& target) {
    std::lock_guard lock {_mutex};

    if (_queue.empty()) return false;
    target = std::move(_queue.front());
    _queue.pop();
    return true;
}

template <typename T>
bool ConcurrentQueue<T>::empty() const {
    std::lock_guard lock {_mutex};

    return _queue.empty();
}

template <typename T>
size_t ConcurrentQueue<T>::size() const {
    std::lock_guard lock {_mutex};

    return _queue.size();
}

template <typename T>
void ConcurrentQueue<T>::clear() {
    std::lock_guard lock {_mutex};

    _queue = {};
}
}