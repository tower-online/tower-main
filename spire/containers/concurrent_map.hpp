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

    void insert_or_assign(const KeyType& key, const ValueType& value) {
        std::unique_lock lock {mutex_};
        map_.insert_or_assign(key, value);
    }

    void insert_or_assign(const KeyType& key, ValueType&& value) {
        std::unique_lock lock {mutex_};
        map_.insert_or_assign(key, std::forward<ValueType>(value));
    }

    ValueType at(const KeyType& key) const {
        return map_.at(key);
    }

    void erase(const KeyType& key) {
        std::unique_lock lock {mutex_};
        if (!map_.contains(key)) return;
        map_.erase(key);
    }

    void clear() {
        std::unique_lock lock {mutex_};
        map_.clear();
    }

    bool contains(const KeyType& key) const {
        std::shared_lock lock {mutex_};
        return map_.contains(key);
    }

    template <typename Function, typename... Args>
        requires std::invocable<Function, ValueType&, Args&&...>
    void apply(const KeyType& key, Function function, Args&&... args) {
        std::shared_lock lock {mutex_};

        if (!map_.contains(key)) return;
        function(map_[key], std::forward<Args>(args)...);
    }

    template <typename Function, typename... Args>
        requires std::invocable<Function, ValueType&, Args...>
    void apply_all(Function function, Args&&... args) {
        std::shared_lock lock {mutex_};

        for (auto& [_, value] : map_) {
            function(value, args...);
        }
    }

    template <typename Filter, typename Function, typename... Args>
        requires std::invocable<Filter, const ValueType&>
        && std::same_as<bool, std::invoke_result_t<Filter, const ValueType&>>
        && std::invocable<Function, ValueType&, Args...>
    void apply_some(Filter filter, Function function, Args&&... args) {
        std::shared_lock lock {mutex_};

        for (auto& [_, value] : map_) {
            if (!filter(value)) continue;

            function(value, args...);
        }
    }

    bool empty() const {
        std::shared_lock lock {mutex_};
        return map_.empty();
    }

private:
    std::unordered_map<KeyType, ValueType> map_ {};
    mutable std::shared_mutex mutex_ {};
};
}