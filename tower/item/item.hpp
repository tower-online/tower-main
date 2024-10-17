#pragma once

#include <tower/network/packet/item_types.hpp>

#include <string_view>
#include <unordered_map>

namespace tower::item {
using namespace network::packet;

class Item {
public:
    explicit Item(const ItemType type)
        : type {type} {}

    virtual ~Item() = default;

    static ItemType item_name_to_type(const std::string_view name);

    const ItemType type;
};

inline ItemType Item::item_name_to_type(const std::string_view name) {
    static std::unordered_map<std::string_view, ItemType> types {
        {"gold", ItemType::GOLD},
        {"fist", ItemType::FIST}};

    if (const auto type_iter {types.find(name)}; type_iter == types.end()) return ItemType::NONE;
    else return type_iter->second;
}
}
