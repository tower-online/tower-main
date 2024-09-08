#pragma once

#include <glm/vec2.hpp>

#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

namespace tower::world {
class Node : public std::enable_shared_from_this<Node> {
public:
    Node() = default;
    Node(const glm::vec2& position, float rotation);
    virtual ~Node() = default;

    void add_child(const std::shared_ptr<Node>& child);
    const std::vector<std::shared_ptr<Node>>& get_childs() const;
    std::shared_ptr<Node> get_parent();
    void set_parent(const std::shared_ptr<Node>& parent);
    std::shared_ptr<Node> get_root();

    glm::vec2 get_global_position() const;
    float get_global_rotation() const;

    const uint32_t node_id {++_id_generator};
    glm::vec2 position {};
    float rotation {};

protected:
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> childs {};

private:
    inline static std::atomic<uint32_t> _id_generator {0};
};


inline Node::Node(const glm::vec2& position, const float rotation)
    : position {position}, rotation {rotation} {}

inline void Node::add_child(const std::shared_ptr<Node>& child) {
    if (!child) return;

    childs.push_back(child);
    child->set_parent(shared_from_this());
}

inline void Node::set_parent(const std::shared_ptr<Node>& parent) {
    this->parent = parent;
}

inline std::shared_ptr<Node> Node::get_root() {
    if (const auto p = parent.lock()) {
        return p->get_root();
    }
    return shared_from_this();
}

inline const std::vector<std::shared_ptr<Node>>& Node::get_childs() const {
    return childs;
}

inline std::shared_ptr<Node> Node::get_parent() {
    return parent.lock();
}

inline glm::vec2 Node::get_global_position() const {
    if (const auto p = parent.lock()) {
        const auto parent_position = p->get_global_position();
        const auto parent_rotation = p->get_global_rotation();
        if (parent_rotation == 0.0f) return parent_position + position;

        const glm::vec2 rotated_position = {
            position.x * std::cos(parent_rotation) - position.y * std::sin(parent_rotation),
            position.x * std::sin(parent_rotation) + position.y * std::cos(parent_rotation)
        };
        return parent_position + rotated_position;
    }
    return position;
}

inline float Node::get_global_rotation() const {
    if (const auto p = parent.lock()) {
        return p->get_global_rotation() + rotation;
    }
    return rotation;
}
}
