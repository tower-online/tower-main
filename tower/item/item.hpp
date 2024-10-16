#pragma once

#include <string_view>
#include <unordered_map>

namespace tower::item {
enum class ItemRarity {NORMAL, MAGIC, RARE, LEGENDARY, UNIQUE};
enum class ItemType {NONE, GOLD, FIST};

class Item {
public:
    virtual ~Item() = default;
};

static ItemType item_name_to_type(const std::string_view name) {
    static std::unordered_map<std::string_view, ItemType> types {
        {"gold", ItemType::GOLD},
        {"fist", ItemType::FIST}};

    if (const auto type_iter {types.find(name)}; type_iter == types.end()) return ItemType::NONE;
    else return type_iter->second;
}
}
