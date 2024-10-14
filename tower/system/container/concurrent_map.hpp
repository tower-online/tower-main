#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace tower {
template <typename KeyType, typename ValueType>
class ConcurrentMap {
    using MapType = std::unordered_map<KeyType, ValueType>;

public:
    struct SharedGuard {
        SharedGuard(MapType& map, std::shared_lock<std::shared_mutex>&& lock)
            : map {map}, _lock {std::move(lock)} {}

        MapType& map;

    private:
        std::shared_lock<std::shared_mutex> _lock;
    };

    struct UniqueGuard {
        UniqueGuard(MapType& map, std::unique_lock<std::shared_mutex>&& lock)
            : map {map}, _lock {std::move(lock)} {}

        MapType& map;

    private:
        std::unique_lock<std::shared_mutex> _lock;
    };

    ConcurrentMap() = default;
    ~ConcurrentMap() = default;
    ConcurrentMap(const ConcurrentMap&) = delete;
    ConcurrentMap& operator=(const ConcurrentMap&) = delete;

    void insert_or_assign(const KeyType& key, const ValueType& value);
    void insert_or_assign(const KeyType& key, ValueType&& value);
    auto erase(const KeyType& key);
    bool try_extract(const KeyType& key, ValueType& target);
    void clear();

    bool try_at(const KeyType& key, ValueType& target) const;
    bool contains(const KeyType& key) const;
    size_t size() const;
    bool empty() const;

    SharedGuard get_shared_guard();
    UniqueGuard get_unique_guard();

private:
    MapType _map {};
    mutable std::shared_mutex _mutex {};
};


template <typename KeyType, typename ValueType>
void ConcurrentMap<KeyType, ValueType>::insert_or_assign(const KeyType& key, const ValueType& value) {
    std::unique_lock lock {_mutex};
    _map.insert_or_assign(key, value);
}

template <typename KeyType, typename ValueType>
void ConcurrentMap<KeyType, ValueType>::insert_or_assign(const KeyType& key, ValueType&& value) {
    std::unique_lock lock {_mutex};
    _map.insert_or_assign(key, std::forward<ValueType>(value));
}

template <typename KeyType, typename ValueType>
auto ConcurrentMap<KeyType, ValueType>::erase(const KeyType& key) {
    std::unique_lock lock {_mutex};
    return _map.erase(key);
}

template <typename KeyType, typename ValueType>
bool ConcurrentMap<KeyType, ValueType>::try_extract(const KeyType& key, ValueType& target) {
    std::unique_lock lock {_mutex};

    auto it = _map.find(key);
    if (it == _map.end()) return false;

    target = std::move(it->second);
    _map.erase(it);
    return true;
}

template <typename KeyType, typename ValueType>
void ConcurrentMap<KeyType, ValueType>::clear() {
    std::unique_lock lock {_mutex};
    _map.clear();
}

template <typename KeyType, typename ValueType>
bool ConcurrentMap<KeyType, ValueType>::try_at(const KeyType& key, ValueType& target) const {
    std::shared_lock lock {_mutex};
    if (!_map.contains(key)) return false;
    target = _map.at(key);
    return true;
}

template <typename KeyType, typename ValueType>
bool ConcurrentMap<KeyType, ValueType>::contains(const KeyType& key) const {
    std::shared_lock lock {_mutex};
    return _map.contains(key);
}

template <typename KeyType, typename ValueType>
size_t ConcurrentMap<KeyType, ValueType>::size() const {
    std::shared_lock lock {_mutex};
    return _map.size();
}

template <typename KeyType, typename ValueType>
bool ConcurrentMap<KeyType, ValueType>::empty() const {
    std::shared_lock lock {_mutex};
    return _map.empty();
}

template <typename KeyType, typename ValueType>
typename ConcurrentMap<KeyType, ValueType>::SharedGuard ConcurrentMap<KeyType, ValueType>::get_shared_guard() {
    return SharedGuard {_map, std::shared_lock {_mutex}};
}

template <typename KeyType, typename ValueType>
typename ConcurrentMap<KeyType, ValueType>::UniqueGuard ConcurrentMap<KeyType, ValueType>::get_unique_guard() {
    return UniqueGuard {_map, std::unique_lock {_mutex}};
}
}
