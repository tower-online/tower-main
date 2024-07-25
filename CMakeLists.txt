cmake_minimum_required(VERSION 3.28)
project(SpireServer
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
add_executable(spire_server
    spire/main.cpp

    spire/container/concurrent_map.hpp
    spire/container/concurrent_queue.hpp

    spire/game/equipment/equipment.hpp
    spire/game/equipment/fist.hpp
    spire/game/player/player.hpp
    spire/game/player/player.cpp
    spire/game/player/inventory.hpp
    spire/game/player/inventory.cpp
    spire/game/entity.hpp
    spire/game/item.hpp

    spire/net/client.hpp
    spire/net/client.cpp
    spire/net/connection.hpp
    spire/net/listener.hpp
    spire/net/packet.hpp
    spire/net/server.hpp
    spire/net/server.cpp
    spire/net/zone.hpp
    spire/net/zone.cpp

    spire/system/event.hpp

    spire/world/collision/circle_collision_shape.hpp
    spire/world/collision/collision_object.hpp
    spire/world/collision/collision_shape.hpp
    spire/world/collision/rectangle_collision_shape.hpp
    spire/world/node.hpp
    spire/world/world.cpp
    spire/world/world.hpp
)
target_compile_features(spire_server PRIVATE cxx_std_20)
target_link_libraries(spire_server PRIVATE Boost::system spdlog flatbuffers glm)
target_include_directories(spire_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(spire/net/packet/schemas)
add_spire_packets_target("spire/net/packet/schemas" "spire/net/packet")
add_dependencies(spire_server spire_packets)

### TESTS ###
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(spire-server-test
    spire/system/event.test.cpp
)
target_compile_features(spire-server-test PRIVATE cxx_std_20)
target_link_libraries(spire-server-test PRIVATE Catch2::Catch2WithMain glm)
target_include_directories(spire-server-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})