#pragma once

#include <tower/world/path_finder.hpp>
#include <tower/world/collision/circle_collision_shape.hpp>
#include <tower/world/collision/collision_object.hpp>
#include <tower/world/collision/rectangle_collision_shape.hpp>
#include <tower/world/entity/entity.hpp>

#include <spdlog/spdlog.h>

namespace tower::game {
using namespace world;

class Piggy : public Entity {
    enum class State { IDLE, FOLLOWING };

public:
    Piggy();

    static std::shared_ptr<Entity> create();
    void tick(Subworld& subworld) override;

private:
    State _state {State::IDLE};
    std::shared_ptr<CircleCollisionShape> _sensor {std::make_shared<CircleCollisionShape>(100.0f)};
    std::deque<Point> _target_path;
};

inline Piggy::Piggy()
    : Entity {EntityType::PIGGY} {}

inline std::shared_ptr<Entity> Piggy::create() {
    auto piggy = std::make_shared<Piggy>();

    piggy->movement_speed_base = 5.0f;
    piggy->resource.max_health = 50.0f;
    piggy->resource.health = piggy->resource.max_health;

    const auto body_collider = CollisionObject::create(
        std::make_shared<RectangleCollisionShape>(glm::vec2 {12, 12}),
        static_cast<uint32_t>(ColliderLayer::ENTITIES),
        static_cast<uint32_t>(ColliderLayer::PLAYERS)
    );
    piggy->add_child(body_collider);

    piggy->add_child(piggy->_sensor);

    return piggy;
}

inline void Piggy::tick(Subworld& subworld) {
    if (_state == State::IDLE) {
        // Check if sensor is colliding with any player
        const auto players = subworld.get_collisions(_sensor.get(), static_cast<uint32_t>(ColliderLayer::PLAYERS));
        if (players.empty()) return;

        auto path = Pathfinder::find_path<Tile>(
            subworld.get_tilemap().get_grid(),
            TileMap::position_to_point(position),
            TileMap::position_to_point(players[0]->get_root()->position));

        _target_path = std::deque<Point> {path.begin(), path.end()};
        _state = State::FOLLOWING;
    } else if (_state == State::FOLLOWING) {
        spdlog::debug("Piggy following {} points", _target_path.size());
        if (_target_path.empty()) {
            _state = State::IDLE;
            return;
        }

        if (TileMap::position_to_point(position) == _target_path.front()) {
            _target_path.pop_front();
        }
        target_direction = _target_path.empty()
            ? glm::vec2 {0, 0}
            : normalize(TileMap::point_to_position(_target_path.front()) - position);
    }
}

}
