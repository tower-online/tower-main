#pragma once

#include <tower/entity/entity.hpp>
#include <tower/physics/sphere_collision_shape.hpp>
#include <tower/item/equipment/weapon/fist.hpp>
#include <tower/system/container/grid.hpp>

namespace tower::entity {
using namespace std::chrono;

class SimpleMonster final : public Entity {
    enum class State {IDLE, CHASING, ATTACKING};

public:
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
    size_t _chasing_path_index;
    steady_clock::time_point _chasing_started;
};
}