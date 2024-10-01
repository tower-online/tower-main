#pragma once

#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>

namespace tower::world {
/**
 * Y-axis aligned 3D node.
 */
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

    glm::vec3 position {};
    float rotation {};

protected:
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children {};
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
    if (const auto p {parent.lock()}) {
        return p->get_root();
    }
    return shared_from_this();
}

inline const std::vector<std::shared_ptr<Node>>& Node::get_children() const {
    return children;
}

inline std::shared_ptr<Node> Node::get_parent() {
    return parent.lock();
}

inline glm::vec3 Node::get_global_position() const {
    if (const auto p {parent.lock()}) {
        const auto parent_position {p->get_global_position()};
        const auto parent_rotation {p->get_global_rotation()};
        const glm::quat quaternion {angleAxis(parent_rotation, glm::vec3(0, 1 ,0))};

        return parent_position + quaternion * position;
    }
    return position;
}

inline float Node::get_global_rotation() const {
    if (const auto p {parent.lock()}) {
        return p->get_global_rotation() + rotation;
    }
    return rotation;
}
}
