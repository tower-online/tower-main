#pragma once

#include <tower/physics/collision_shape.hpp>

namespace tower::item {
class MeleeAttackable {
public:
    virtual ~MeleeAttackable() = default;

    int melee_attack_damage {};
    const physics::CollisionShape* melee_attack_shape() const { return _melee_attack_shape.get(); }

protected:
    std::shared_ptr<physics::CollisionShape> _melee_attack_shape;
};
}