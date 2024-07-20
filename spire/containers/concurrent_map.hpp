#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace spire {
template <typename KeyType, typename ValueType>
class ConcurrentMap {
public:
    ConcurrentMap() = default;
    ~ConcurrentMap() = default;
    ConcurrentMap(const ConcurrentMap&) = delete;
    ConcurrentMap& operator=(const ConcurrentMap&) = delete;

    void insert_or_assign(const KeyType& key, const ValueType& value);
    void insert_or_assign(const KeyType& key, ValueType&& value);
    void erase(const KeyType& key);
    bool try_extract(const KeyType& key, ValueType& target);
    void clear();

    ValueType at(const KeyType& key) const;
    bool contains(const KeyType& key) const;
    bool empty() const;

    template <typename Function, typename... Args>
        requires std::invocable<Function, ValueType&, Args&&...>
    void apply(const KeyType& key, Function function, Args&&... args);

    template <typename Function, typename... Args>
        requires std::invocable<Function, ValueType&, Args...>
    void apply_all(Function function, Args&&... args);

    template <typename Filter, typename Function, typename... Args>
        requires std::invocable<Filter, const ValueType&>
        && std::same_as<bool, std::invoke_result_t<Filter, const ValueType&>>
        && std::invocable<Function, ValueType&, Args...>
    void apply_some(Filter filter, Function function, Args&&... args);

private:
    std::unordered_map<KeyType, ValueType> _map {};
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
ValueType ConcurrentMap<KeyType, ValueType>::at(const KeyType& key) const {
    return _map.at(key);
}

template <typename KeyType, typename ValueType>
void ConcurrentMap<KeyType, ValueType>::erase(const KeyType& key) {
    std::unique_lock lock {_mutex};
    if (!_map.contains(key)) return;
    _map.erase(key);
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
bool ConcurrentMap<KeyType, ValueType>::contains(const KeyType& key) const {
    std::shared_lock lock {_mutex};
    return _map.contains(key);
}

template <typename KeyType, typename ValueType>
template <typename Function, typename ... Args> requires std::invocable<Function, ValueType&, Args&&...>
void ConcurrentMap<KeyType, ValueType>::apply(const KeyType& key, Function function, Args&&... args) {
    std::shared_lock lock {_mutex};

    if (!_map.contains(key)) return;
    function(_map[key], std::forward<Args>(args)...);
}

template <typename KeyType, typename ValueType>
template <typename Function, typename ... Args> requires std::invocable<Function, ValueType&, Args...>
void ConcurrentMap<KeyType, ValueType>::apply_all(Function function, Args&&... args) {
    std::shared_lock lock {_mutex};

    for (auto& [_, value] : _map) {
        function(value, args...);
    }
}

template <typename KeyType, typename ValueType>
template <typename Filter, typename Function, typename ... Args> requires std::invocable<Filter, const ValueType&> &&
    std::same_as<bool, std::invoke_result_t<Filter, const ValueType&>> && std::invocable<Function, ValueType&, Args...>
void ConcurrentMap<KeyType, ValueType>::apply_some(Filter filter, Function function, Args&&... args) {
    std::shared_lock lock {_mutex};

    for (auto& [_, value] : _map) {
        if (!filter(value)) continue;

        function(value, args...);
    }
}

template <typename KeyType, typename ValueType>
bool ConcurrentMap<KeyType, ValueType>::empty() const {
    std::shared_lock lock {_mutex};
    return _map.empty();
}
}
