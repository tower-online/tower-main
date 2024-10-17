#pragma once

#include <tower/item/item.hpp>
#include <tower/world/world_object.hpp>

namespace tower::item {
class ItemObject : public world::WorldObject {
public:
    explicit ItemObject(std::unique_ptr<Item> item)
        : item {std::move(item)} {}

    std::unique_ptr<Item> item;
};
}