#include <catch2/catch_test_macros.hpp>
#include <spire/collision/rectangle_shape.hpp>

using namespace spire;

TEST_CASE("RectangleShape checks collisions", "RectangleShape") {
    RectangleShape r1 {glm::vec2 {0, 0}, glm::vec2 {4, 4}};
    RectangleShape r2 {glm::vec2 {5, 5}, glm::vec2 {3, 3}};
    RectangleShape r3 {glm::vec2 {0, 0}, glm::vec2 {3, 3}};
    RectangleShape r4 {glm::vec2 {5, 5}, glm::vec2 {1, 1}};

    REQUIRE(r1.is_colliding(r2));
    REQUIRE(r2.is_colliding(r1));
    REQUIRE(r1.is_colliding(r3));
    REQUIRE(r4.is_colliding(r1));
    REQUIRE(!r3.is_colliding(r4));
}