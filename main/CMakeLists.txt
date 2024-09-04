cmake_minimum_required(VERSION 3.28)
project(TowerServer
    VERSION 0.1
    LANGUAGES CXX
)

option(TOWER_BUILD_TESTS "Enable the build of tests" ON)

### DEPENDENCIES ###
# Boost.asio -> Boost.system
# Boost.mysql -> Boost.charconv
find_package(Boost 1.85.0 REQUIRED COMPONENTS charconv system thread)

#find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v24.3.25
)
FetchContent_MakeAvailable(flatbuffers)
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable the build of flatbuffers tests" FORCE)

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

FetchContent_Declare(jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG v0.7.0
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "Disable the build of jwt-cpp examples" FORCE)
FetchContent_MakeAvailable(jwt-cpp)


### TARGETS ###
add_library(libtower STATIC
    tower/item/item.hpp
    tower/item/equipment/equipment.hpp
    tower/item/equipment/fist.hpp

    tower/network/client.hpp
    tower/network/client.cpp
    tower/network/connection.hpp
    tower/network/connection.cpp
    tower/network/db.hpp
    tower/network/listener.hpp
    tower/network/packet.hpp
    tower/network/server.hpp
    tower/network/server.cpp
    tower/network/zone.hpp
    tower/network/zone.cpp

    tower/player/player.hpp
    tower/player/player.cpp
    tower/player/inventory.hpp
    tower/player/inventory.cpp

    tower/system/math.hpp
    tower/system/probe.hpp
    tower/system/settings.hpp
    tower/system/settings.cpp
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
add_library(tower::libtower ALIAS libtower)

target_compile_features(libtower PUBLIC cxx_std_20)
target_link_libraries(libtower
    PUBLIC
    spdlog
    flatbuffers
    glm
    Boost::system

    PRIVATE
    Boost::charconv
    Boost::thread
#    OpenSSL::SSL
    jwt-cpp::jwt-cpp
)
target_include_directories(libtower PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(libtower PUBLIC
    TOWER_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    TILE_MAP_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/bin/tile_map"
)

add_executable(tower-server
    tower/main.cpp
)
target_link_libraries(tower-server PRIVATE tower::libtower)


### SCHEMAS ###
add_subdirectory(schema)
tower_add_packet_schemas("${CMAKE_CURRENT_SOURCE_DIR}/schema/packet" "${CMAKE_CURRENT_SOURCE_DIR}/tower/network/packet")
tower_add_world_schemas("${CMAKE_CURRENT_SOURCE_DIR}/schema/world" "${CMAKE_CURRENT_SOURCE_DIR}/tower/world/data")
#add_dependencies(libtower tower-schema-packet tower-schema-world)


### TESTS ###
if (TOWER_BUILD_TESTS)
    # unit tests
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(tower-unit-tests
        tower/world/path_finder.test.cpp
    )
    target_compile_features(tower-unit-tests PRIVATE cxx_std_20)
    target_link_libraries(tower-unit-tests PRIVATE Catch2::Catch2WithMain tower::libtower)
endif ()