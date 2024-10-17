#pragma once

#include <tower/item/item.hpp>

namespace tower::item {
class Gold : public Item {
public:
    Gold()
        : Item {ItemType::GOLD} {}

    int amount {};
};
}