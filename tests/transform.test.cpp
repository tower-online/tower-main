#include <catch2/catch_test_macros.hpp>
#include <tower/world/world_object.hpp>

using namespace tower;
using namespace tower::world;

TEST_CASE("Node rotates relative to its parent") {
    auto node1 {std::make_shared<WorldObject>()};
    auto node2 {std::make_shared<WorldObject>()};
    node1->add_child(node2);

    node1->position = glm::vec3 {2, 0, 0};
    node2->position = glm::vec3 {1, 0, 0};
    REQUIRE(node2->get_global_position() == glm::vec3 {3, 0, 0});

    node1->rotation = std::numbers::pi;
    REQUIRE(all(epsilonEqual(node2->get_global_position(), glm::vec3 {1, 0, 0}, 1e-5f)));
}