#pragma once

#include <tower/item/equipment/melee_attackable.hpp>
#include <tower/item/equipment/equipment.hpp>
#include <tower/physics/cube_collision_shape.hpp>

#include <chrono>

namespace tower::item {
using namespace std::chrono;

class Fist final : public Equipment, public MeleeAttackable {
public:
    class Data;

    Fist();

    static std::unique_ptr<Fist> create();

    std::shared_ptr<world::WorldObject> node;
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
