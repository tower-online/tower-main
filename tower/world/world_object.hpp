#pragma once

#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tower/units.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace tower::world {
inline static constexpr glm::vec3 zero3 {0, 0, 0};
inline static constexpr glm::vec3 forward {0, 0, 1};
inline static constexpr glm::vec3 up {0, 1, 0};

class WorldObject : public std::enable_shared_from_this<WorldObject> {
public:
    WorldObject() = default;
    WorldObject(const glm::vec3& position, float rotation);
    virtual ~WorldObject() = default;

    void add_child(const std::shared_ptr<WorldObject>& child);
    const std::vector<std::shared_ptr<WorldObject>>& get_children() const;
    std::shared_ptr<WorldObject> get_parent();
    void set_parent(const std::shared_ptr<WorldObject>& parent);
    std::shared_ptr<WorldObject> get_root();

    glm::vec3 get_global_position() const;
    float get_global_rotation() const;

public:
    const uint32_t object_id {++id_generator};
    glm::vec3 position {};
    radian rotation {};

protected:
    std::weak_ptr<WorldObject> parent;
    std::vector<std::shared_ptr<WorldObject>> children {};

private:
    inline static std::atomic<uint32_t> id_generator {0};
};

inline WorldObject::WorldObject(const glm::vec3& position, const float rotation)
    : position {position}, rotation {rotation} {}

inline void WorldObject::add_child(const std::shared_ptr<WorldObject>& child) {
    if (!child) return;

    children.push_back(child);
    child->set_parent(shared_from_this());
}

inline void WorldObject::set_parent(const std::shared_ptr<WorldObject>& parent) {
    this->parent = parent;
}

inline std::shared_ptr<WorldObject> WorldObject::get_root() {
    auto node {shared_from_this()};

    while (true) {
        auto p {node->parent.lock()};
        if (!p) return node;
        node.swap(p);
    }
}

inline const std::vector<std::shared_ptr<WorldObject>>& WorldObject::get_children() const {
    return children;
}

inline std::shared_ptr<WorldObject> WorldObject::get_parent() {
    return parent.lock();
}

inline glm::vec3 WorldObject::get_global_position() const {
    glm::vec3 current_position {position};
    auto p {parent.lock()};

    while (p) {
        const glm::quat quaternion {angleAxis(p->rotation, up)};
        current_position = p->position + quaternion * current_position;

        p = p->parent.lock();
    }

    return current_position;
}

inline float WorldObject::get_global_rotation() const {
    float current_rotation {rotation};
    auto p {parent.lock()};

    while (p) {
        current_rotation += p->rotation;
        p = p->parent.lock();
    }

    return current_rotation;
}
}
