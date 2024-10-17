#pragma once

#include <tower/item/item.hpp>

namespace tower::item {
class Equipment : public Item {
public:
    explicit Equipment(const ItemType type)
        : Item {type} {}
};
}
