#pragma once

#include <tower/item/item.hpp>
#include <tower/item/stackable.hpp>

namespace tower::item {
class Gold : public Item, public Stackable {
public:
    Gold()
        : Item {ItemType::GOLD} {}
};
}