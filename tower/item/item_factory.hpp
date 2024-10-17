#pragma once

#include <tower/item/item.hpp>

#include <memory>

namespace tower::item {
class ItemFactory {
public:
    struct ItemCreateConfig {
        ItemType type;
        ItemRarity rarity_min;
        ItemRarity rarity_max;
    };

    static std::unique_ptr<Item> create(const ItemCreateConfig& config);
};
}