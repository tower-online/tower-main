#pragma once

#include <tower/item/equipment/weapon/melee_attackable.hpp>
#include <tower/item/equipment/weapon/weapon.hpp>
#include <tower/physics/cube_collision_shape.hpp>

#include <chrono>

namespace tower::item::equipment {
using namespace std::chrono;

class Fist final : public Weapon, public MeleeAttackable {
public:
    class Data;

    static std::shared_ptr<Fist> create();

    std::shared_ptr<world::Node> node;
};

class Fist::Data {
    static constexpr std::string_view file {TOWER_DATA_ROOT "/weapons/fist.json"};
public:
    static void load();

    static milliseconds attack_interval() { return _attack_interval; }
    static const glm::vec3& attack_shape_size() { return _attack_shape_size; }

private:
    inline static milliseconds _attack_interval;
    inline static glm::vec3 _attack_shape_size;
};
}
