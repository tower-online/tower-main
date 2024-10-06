#pragma once

#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tower/units.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace tower::world {
class Node : public std::enable_shared_from_this<Node> {
public:
    Node() = default;
    Node(const glm::vec3& position, float rotation);
    virtual ~Node() = default;

    void add_child(const std::shared_ptr<Node>& child);
    const std::vector<std::shared_ptr<Node>>& get_children() const;
    std::shared_ptr<Node> get_parent();
    void set_parent(const std::shared_ptr<Node>& parent);
    std::shared_ptr<Node> get_root();

    glm::vec3 get_global_position() const;
    float get_global_rotation() const;

public:
    const uint32_t node_id {++id_generator};
    glm::vec3 position {};
    radian rotation {};

protected:
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children {};

private:
    inline static std::atomic<uint32_t> id_generator {0};
};

inline Node::Node(const glm::vec3& position, const float rotation)
    : position {position}, rotation {rotation} {}

inline void Node::add_child(const std::shared_ptr<Node>& child) {
    if (!child) return;

    children.push_back(child);
    child->set_parent(shared_from_this());
}

inline void Node::set_parent(const std::shared_ptr<Node>& parent) {
    this->parent = parent;
}

inline std::shared_ptr<Node> Node::get_root() {
    auto node {shared_from_this()};

    while (true) {
        auto p {node->parent.lock()};
        if (!p) return node;
        node.swap(p);
    }
}

inline const std::vector<std::shared_ptr<Node>>& Node::get_children() const {
    return children;
}

inline std::shared_ptr<Node> Node::get_parent() {
    return parent.lock();
}

inline glm::vec3 Node::get_global_position() const {
    glm::vec3 current_position {position};
    auto p {parent.lock()};

    while (p) {
        const glm::quat quaternion {angleAxis(p->rotation, glm::vec3(0, 1 ,0))};
        current_position = p->position + quaternion * current_position;

        p = p->parent.lock();
    }

    return current_position;
}

inline float Node::get_global_rotation() const {
    float current_rotation {rotation};
    auto p {parent.lock()};

    while (p) {
        current_rotation += p->rotation;
        p = p->parent.lock();
    }

    return current_rotation;
}
}
