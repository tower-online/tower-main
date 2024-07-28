cmake_minimum_required(VERSION 3.28)
project(TowerServer
    VERSION 0.1
    LANGUAGES CXX
)

### DEPENDENCIES ###
include(FetchContent)

find_package(Boost 1.85.0 REQUIRED COMPONENTS system)

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v24.3.25
)
FetchContent_MakeAvailable(flatbuffers)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)


### TARGETS ###
add_executable(tower_server
    tower/main.cpp

    tower/container/concurrent_map.hpp
    tower/container/concurrent_queue.hpp

    tower/game/equipment/equipment.hpp
    tower/game/equipment/fist.hpp
    tower/game/player/player.hpp
    tower/game/player/player.cpp
    tower/game/player/inventory.hpp
    tower/game/player/inventory.cpp
    tower/game/entity.hpp
    tower/game/item.hpp

    tower/net/client.hpp
    tower/net/client.cpp
    tower/net/connection.hpp
    tower/net/listener.hpp
    tower/net/packet.hpp
    tower/net/server.hpp
    tower/net/server.cpp
    tower/net/zone.hpp
    tower/net/zone.cpp

    tower/system/event.hpp

    tower/world/collision/circle_collision_shape.hpp
    tower/world/collision/collision_object.hpp
    tower/world/collision/collision_shape.hpp
    tower/world/collision/rectangle_collision_shape.hpp
    tower/world/node.hpp
    tower/world/tile_map.hpp
    tower/world/world.cpp
    tower/world/world.hpp
)
target_compile_features(tower_server PRIVATE cxx_std_20)
target_link_libraries(tower_server PRIVATE Boost::system spdlog flatbuffers glm)
target_include_directories(tower_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tower/net/packet/schemas)
add_tower_packets_target("tower/net/packet/schemas" "tower/net/packet")
add_dependencies(tower_server tower_packets)

### TESTS ###
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(tower_server_test
    tower/system/event.test.cpp
)
target_compile_features(tower_server_test PRIVATE cxx_std_20)
target_link_libraries(tower_server_test PRIVATE Catch2::Catch2WithMain glm)
target_include_directories(tower_server_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})