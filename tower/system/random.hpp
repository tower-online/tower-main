#pragma once

#include <iterator>
#include <numeric>
#include <random>

namespace tower {
/**
 *
 * @return Random value between [begin, end] (inclusive)
 */
static int random_range(const int min, const int max) {
    thread_local std::mt19937 gen {std::random_device {}()};

    return std::uniform_int_distribution {min, max}(gen);
}

static float random_range(const float min, const float max) {
    thread_local std::mt19937 gen {std::random_device {}()};

    return std::uniform_real_distribution {min, max}(gen);
}

template<std::forward_iterator ItemIterator>
requires std::sentinel_for<ItemIterator, ItemIterator>
static ItemIterator random_select(ItemIterator item_begin, ItemIterator item_end) {
    thread_local std::mt19937 gen {std::random_device {}()};

    const auto distance {std::distance(item_begin, item_end)};
    if (distance <= 0) return item_end;

    const auto advance {std::uniform_int_distribution {0, distance - 1}(gen)};
    std::advance(item_begin, advance);

    return item_begin;
}

template<std::forward_iterator ItemIterator, std::forward_iterator WeightIterator>
requires std::sentinel_for<ItemIterator, ItemIterator> &&
    std::sentinel_for<WeightIterator, WeightIterator> &&
    std::is_arithmetic_v<typename std::iterator_traits<WeightIterator>::value_type>
static ItemIterator random_select(
    ItemIterator item_begin, ItemIterator item_end, WeightIterator weight_begin, WeightIterator weight_end) {
    using WeightValueType = typename std::iterator_traits<WeightIterator>::value_type;

    thread_local std::mt19937 gen {std::random_device {}()};

    const auto item_distance {std::distance(item_begin, item_end)};
    const auto weight_distance {std::distance(weight_begin, weight_end)};

    if (item_distance <= 0 || weight_distance <= 0 || item_distance != weight_distance) return item_end;

    const WeightValueType weight_total {std::accumulate(weight_begin, weight_end, WeightValueType {0})};
    WeightValueType weight_target;
    if constexpr (std::is_floating_point_v<WeightValueType>) {
        weight_target = static_cast<WeightValueType>(std::uniform_real_distribution {0.0, static_cast<double>(weight_total)}(gen));
    } else {
        weight_target = static_cast<WeightValueType>(std::uniform_int_distribution {0, weight_total - 1}(gen));
    }
    WeightValueType weight_sum {0};

    while (item_begin != item_end) {
        weight_sum += *weight_begin;

        if (weight_sum > weight_target) break;

        std::advance(item_begin, 1);
        std::advance(weight_begin, 1);
    }

    return item_begin;
}
}
