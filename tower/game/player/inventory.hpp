#pragma once

#include <tower/game/equipment/equipment.hpp>

#include <memory>

namespace tower::game::player {
using namespace game::equipment;

class Inventory {
public:
    std::shared_ptr<Equipment>& get_main_weapon();
    void set_main_weapon(std::shared_ptr<Equipment> weapon);

private:
    std::shared_ptr<Equipment> _main_weapon;
};
}
