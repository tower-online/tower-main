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
add_executable(tower-server
    tower/app.cpp

    tower/item/item.hpp
    tower/item/equipment/equipment.hpp
    tower/item/equipment/fist.hpp

    tower/net/client.hpp
    tower/net/connection.hpp
    tower/net/listener.hpp
    tower/net/packet.hpp
    tower/net/server.hpp
    tower/net/server.cpp
    tower/net/zone.hpp
    tower/net/zone.cpp

    tower/player/player.hpp
    tower/player/player.cpp
    tower/player/inventory.hpp
    tower/player/inventory.cpp

    tower/system/event.hpp
    tower/system/math.hpp
    tower/system/timer.hpp
    tower/system/container/concurrent_map.hpp
    tower/system/container/concurrent_queue.hpp
    tower/system/container/grid.hpp

    tower/world/node.hpp
    tower/world/path_finder.hpp
    tower/world/subworld.cpp
    tower/world/subworld.hpp
    tower/world/tile_map.hpp
    tower/world/world_generator.hpp
    tower/world/world_generator.cpp
    tower/world/collision/circle_collision_shape.hpp
    tower/world/collision/circle_collision_shape.cpp
    tower/world/collision/collision_object.hpp
    tower/world/collision/collision_shape.hpp
    tower/world/collision/rectangle_collision_shape.hpp
    tower/world/collision/rectangle_collision_shape.cpp
    tower/world/entity/entity.hpp
    tower/world/entity/entity_manager.hpp
    tower/world/entity/mob/piggy.hpp
)
target_compile_features(tower-server PRIVATE cxx_std_20)
target_link_libraries(tower-server PRIVATE Boost::system spdlog flatbuffers glm)
target_include_directories(tower-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(tower-server PRIVATE
    TILE_MAP_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/bin/tile_map"
    ZONE_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/bin/zone"
)


### SCHEMAS ###
add_subdirectory(tower/net/packet/schema)
add_tower_packet_target("tower/net/packet/schema" "tower/net/packet")
add_dependencies(tower-server tower-packet)

file(GLOB world_schemas tower/world/schema/*.fbs)
add_custom_target(
    tower-world-schema
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND flatc -o tower/world/schema --cpp --cpp-std c++17 --filename-suffix "\"\"" --filename-ext hpp ${world_schemas}
    DEPENDS flatc
)
add_dependencies(tower-server tower-world-schema)
message("world_schemas: ${world_schemas}")

### TESTS ###
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(tower-server-test
    tower/system/event.test.cpp
    tower/world/path_finder.test.cpp
)
target_compile_features(tower-server-test PRIVATE cxx_std_20)
target_link_libraries(tower-server-test PRIVATE Catch2::Catch2WithMain glm)
target_include_directories(tower-server-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})