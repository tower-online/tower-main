#pragma once

#include <spire/game/entity.hpp>

#include <memory>

namespace spire::game {
class Player : public Entity, public std::enable_shared_from_this<Player> {
};
}