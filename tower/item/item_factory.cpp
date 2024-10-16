#include <tower/item/gold.hpp>
#include <tower/item/item_factory.hpp>
#include <tower/item/equipment/weapon/fist.hpp>

namespace tower::item {
std::unique_ptr<Item> ItemFactory::create(const ItemCreateConfig& config) {
    if (config.type == ItemType::GOLD) {
        auto gold {std::make_unique<Gold>()};
        gold->amount = 1;
        return gold;
    }
    if (config.type == ItemType::FIST) {
        auto fist {equipment::Fist::create()};
        return fist;
    }

    return {};
}
}
