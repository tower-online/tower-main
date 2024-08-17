cmake_minimum_required(VERSION 3.28)
project(TowerServer
    VERSION 0.1
    LANGUAGES CXX
)

option(TOWER_BUILD_TESTS "Enable the build of tests" ON)
option(TOWER_BUILD_DUMMY "Enable the build of dummy client" ON)

### DEPENDENCIES ###
include(FetchContent)

find_package(Python REQUIRED COMPONENTS Development)
find_package(Boost 1.84.0 REQUIRED COMPONENTS python3 system thread)
find_package(OpenSSL REQUIRED)

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

FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.9.2
)
FetchContent_MakeAvailable(libpqxx)
set(SKIP_BUILD_TEST ON CACHE BOOL "Disable the build of libpqxx tests" FORCE)


### TARGETS ###
add_library(libtower STATIC
    tower/item/item.hpp
    tower/item/equipment/equipment.hpp
    tower/item/equipment/fist.hpp

    tower/net/client.hpp
    tower/net/client.cpp
    tower/net/connection.hpp
    tower/net/connection.cpp
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
    tower/system/jwt.hpp
    tower/system/math.hpp
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
    Python::Python
    spdlog
    flatbuffers
    glm
    libpqxx::pqxx

    PRIVATE
    Boost::python
    Boost::system
    Boost::thread
    OpenSSL::SSL
)
target_include_directories(libtower PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(libtower PUBLIC
    TOWER_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    TILE_MAP_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/bin/tile_map"
    TOWER_LIB_PYTHONPATH="${CMAKE_CURRENT_SOURCE_DIR}/lib/py:$ENV{PYTHONPATH}"
)

add_executable(tower-server
    tower/main.cpp
)
target_link_libraries(tower-server PRIVATE tower::libtower)


### SCHEMAS ###
add_subdirectory(tower/net/packet/schema)

add_tower_packet_target("tower/net/packet/schema" "tower/net/packet")
add_dependencies(libtower tower-packet)

add_tower_world_schema_target("tower/net/packet/schema" "tower/world/schema")
add_dependencies(libtower tower-world-schema)

if (TOWER_BUILD_DUMMY)
    set(schema_path "tower/net/packet/schema")

    file(GLOB schemas
        "${schema_path}/packet/*.fbs"
        "${schema_path}/packet/client/*.fbs"
    )

    add_custom_target(
        dummy-packet
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND flatc -I ${schema_path}/packet -o dummy --python --filename-suffix "\"\"" ${schemas}
        DEPENDS flatc
    )
    message("dummy-packet: ${schemas}")
endif ()

### TESTS ###
if (TOWER_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(tower-unit-tests
        tower/system/event.test.cpp
        tower/system/jwt.test.cpp
        tower/world/path_finder.test.cpp
    )
    target_compile_features(tower-unit-tests PRIVATE cxx_std_20)
    target_link_libraries(tower-unit-tests PRIVATE tower::libtower Catch2::Catch2WithMain)

    add_executable(tower-integration-tests
        test/db/psql_connection.test.cpp
    )
    target_compile_features(tower-integration-tests PRIVATE cxx_std_20)
    target_link_libraries(tower-integration-tests PRIVATE tower::libtower Catch2::Catch2WithMain)
endif ()