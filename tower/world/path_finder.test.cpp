#include <catch2/catch_test_macros.hpp>
#include <tower/world/path_finder.hpp>

using namespace tower;
using namespace tower::world;

TEST_CASE("Pathfinder returns the path", "[Pathfinder]") {
    const Grid<int> grid1 {
        {
            0, 0, 0, 0, 0,
            1, 1, 1, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
        },
        5, 5
    };

    const Grid<int> grid2 {
        {
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
        },
        5, 5
    };

    const Grid<int> grid3 {
        {
            0, 0, 0, 0, 0,
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
        },
        5, 5
    };

    SECTION("A* path found") {
        const auto path1 = Pathfinder::find_path<int>(grid1, {0, 0}, {4, 4}, Pathfinder::Algorithm::ASTAR);
        REQUIRE(!path1.empty());

        const auto path2 = Pathfinder::find_path<int>(grid2, {0, 0}, {4, 4}, Pathfinder::Algorithm::ASTAR);
        REQUIRE(!path2.empty());
    }

    SECTION("A* path not found") {
        const auto path3 = Pathfinder::find_path<int>(grid3, {0, 0}, {4, 4}, Pathfinder::Algorithm::ASTAR);
        REQUIRE(path3.empty());
    }

    SECTION("JPS path found") {
        const auto path1 = Pathfinder::find_path<int>(grid1, {0, 0}, {4, 4}, Pathfinder::Algorithm::JPS);
        REQUIRE(!path1.empty());

        const auto path2 = Pathfinder::find_path<int>(grid2, {0, 0}, {4, 4}, Pathfinder::Algorithm::JPS);
        REQUIRE(!path2.empty());
    }

    SECTION("JPS path not found") {
        const auto path3 = Pathfinder::find_path<int>(grid3, {0, 0}, {4, 4}, Pathfinder::Algorithm::JPS);
        REQUIRE(path3.empty());
    }
}
