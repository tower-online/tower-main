#include <tower/game/player/inventory.hpp>

namespace tower::game::player {
std::shared_ptr<Equipment>& Inventory::get_main_weapon() {
    return _main_weapon;
}

void Inventory::set_main_weapon(std::shared_ptr<Equipment> weapon) {
    _main_weapon = weapon;
}
}
