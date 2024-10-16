#pragma once

#include <tower/entity/entity.hpp>
#include <tower/physics/sphere_collision_shape.hpp>
#include <tower/item/equipment/weapon/fist.hpp>
#include <tower/system/container/grid.hpp>
#include <tower/system/state_machine.hpp>

namespace tower::entity {
using namespace std::chrono;

class SimpleMonster final : public Entity {
    //TODO: Use state machine instead of enum?
    enum class State {IDLE, CHASING, ATTACKING};

public:
    class Data;

    SimpleMonster();

    void tick(network::Zone* zone) override;

    static std::shared_ptr<SimpleMonster> create();

private:
    State _state {State::IDLE};
    std::shared_ptr<item::equipment::Fist> _weapon;
    std::shared_ptr<physics::SphereCollisionShape> _detection_area;
    Entity* _chasing_target {nullptr};
    //TODO: Use queue?
    std::vector<Point> _chasing_path;
    size_t _chasing_path_index {};
    steady_clock::time_point _chasing_started;
};


class SimpleMonster::Data {
    static constexpr std::string_view file {TOWER_DATA_ROOT "/monsters/simple_monster.json"};

public:
    static void load();

    static uint32_t max_health() { return _max_health; }

    static uint32_t exp_amount() { return _exp_amount; }
    static const std::vector<item::ItemType>& drop_items() { return _drop_items; }
    static const std::vector<float>& drop_items_weight() { return _drop_items_weight; }

private:
    inline static uint32_t _max_health;

    inline static uint32_t _exp_amount;
    inline static std::vector<item::ItemType> _drop_items;
    inline static std::vector<float> _drop_items_weight;
};


class IdleState : public State {
    static constexpr std::string_view name {"Idle"};

public:
    void enter() override {}
    void exit() override {}
    std::string_view get_name() const override { return name; }
};

class AttackingState : public State {
    static constexpr std::string_view name {"Attacking"};

public:
    void enter() override {}
    void exit() override {}
    std::string_view get_name() const override { return name; }
};
}