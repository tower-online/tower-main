#include <tower/item/gold.hpp>
#include <tower/item/item_factory.hpp>
#include <tower/item/equipment/fist.hpp>

#include <tower/system/random.hpp>

namespace tower::item {
std::unique_ptr<Item> ItemFactory::create(const ItemCreateConfig& config) {
    if (config.type == ItemType::GOLD) {
        auto gold {std::make_unique<Gold>()};
        gold->amount = random_range(1, 10);
        return gold;
    }
    if (config.type == ItemType::FIST) {
        auto fist {Fist::create()};
        return fist;
    }

    return {};
}
}
