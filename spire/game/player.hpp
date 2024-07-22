#pragma once

#include <spire/game/entity.hpp>

#include <memory>

namespace spire::game {
class Player : public Entity, public std::enable_shared_from_this<Player> {
public:
    constexpr static float MOVEMENT_SPEED = 10.0f;
};
}