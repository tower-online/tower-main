#pragma once

#include <tower/entity/entity.hpp>
#include <tower/physics/collision_object.hpp>
#include <tower/system/container/grid.hpp>

#include <unordered_set>

namespace tower::world {
using namespace tower::entity;
using namespace tower::physics;

class Subworld {
public:
    explicit Subworld(Grid<bool>&& obstacles_grid);

    void tick();

    void add_entity(const std::shared_ptr<Entity>& entity);
    void remove_entity(const std::shared_ptr<Entity>& entity);

    void add_collision_area(const std::shared_ptr<CollisionObject>& area);
    void remove_collision_area(uint32_t area_id);

    void add_collision_objects_from_tree(const std::shared_ptr<Node>& node);
    void remove_collision_objects_from_tree(const std::shared_ptr<Node>& node);

    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const std::shared_ptr<CollisionObject>& collider) const;
    std::vector<std::shared_ptr<CollisionObject>> get_collisions(const CollisionShape* target_shape, uint32_t mask) const;

    const std::unordered_map<uint32_t, std::shared_ptr<Entity>>& entities() const { return _entities; }
    imeters size_x() const { return static_cast<int>(_obstacles_grid.cols); }
    imeters size_z() const { return static_cast<int>(_obstacles_grid.rows); }
    const Grid<bool>& obstacles_grid() const { return _obstacles_grid; }

    static glm::vec3 point2position(const Point& p) { return {p.c + 0.5f, 0, p.r + 0.5f}; }

private:
    std::unordered_map<uint32_t, std::shared_ptr<Entity>> _entities {};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_objects {};
    std::unordered_map<uint32_t, std::shared_ptr<CollisionObject>> _collision_areas {};
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> _contacts {};
    Grid<bool> _obstacles_grid;
};
}
