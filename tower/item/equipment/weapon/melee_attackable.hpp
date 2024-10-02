#pragma once

#include <tower/physics/collision_shape.hpp>

namespace tower::item::equipment {
class MeleeAttackable {
public:
    virtual ~MeleeAttackable() = default;

    physics::CollisionShape* attack_shape() { return _attack_shape.get(); }

protected:
    std::shared_ptr<physics::CollisionShape> _attack_shape;
};
}