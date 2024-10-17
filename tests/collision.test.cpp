#include <catch2/catch_test_macros.hpp>
#include <tower/physics/sphere_collision_shape.hpp>
#include <tower/physics/cube_collision_shape.hpp>

using namespace tower;
using namespace tower::physics;

TEST_CASE("CubeCollisionShape checks collisions") {
    auto cube1 {std::make_shared<CubeCollisionShape>(glm::vec3 {1, 1, 1})}; // 2 * 2 * 2
    auto cube2 {std::make_shared<CubeCollisionShape>(glm::vec3 {1, 1, 1})}; // 2 * 2 * 2

    REQUIRE(cube1->is_colliding(cube1.get()));
    REQUIRE(cube1->is_colliding(cube2.get()));

    cube2->position = glm::vec3 {1, 0, 0};
    REQUIRE(cube1->is_colliding(cube2.get()));

    // cube1 and cube2 abut on the surface
    cube2->position = glm::vec3 {2, 0, 0};
    REQUIRE(cube1->is_colliding(cube2.get()));

    cube2->position = glm::vec3 {3, 0, 0};
    REQUIRE(cube1->is_colliding(cube2.get()) == false);
}

TEST_CASE("SphereCollisionShape checks collisions") {
    auto sphere1 {std::make_shared<SphereCollisionShape>(1)};
    auto sphere2 {std::make_shared<SphereCollisionShape>(1)};

    REQUIRE(sphere1->is_colliding(sphere1.get()));
    REQUIRE(sphere1->is_colliding(sphere2.get()));

    sphere2->position = glm::vec3 {1, 0, 0};
    REQUIRE(sphere1->is_colliding(sphere2.get()));

    // cube1 and cube2 abut
    sphere2->position = glm::vec3 {2, 0, 0};
    REQUIRE(sphere1->is_colliding(sphere2.get()));

    sphere2->position = glm::vec3 {3, 0, 0};
    REQUIRE(sphere1->is_colliding(sphere2.get()) == false);
}