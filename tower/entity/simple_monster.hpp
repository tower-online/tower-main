#pragma once

#include <tower/entity/entity.hpp>
#include <tower/physics/sphere_collision_shape.hpp>
#include <tower/item/equipment/weapon/fist.hpp>

namespace tower::entity {
class SimpleMonster final : public Entity {
    enum class State {IDLE, CHASING};

public:
    void tick(world::Subworld& subworld) override;

    static std::shared_ptr<SimpleMonster> create();

private:
    State _state {State::IDLE};
    std::shared_ptr<item::equipment::Fist> _weapon;
    std::shared_ptr<physics::SphereCollisionShape> _detection_area;
};
}