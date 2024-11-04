cmake_minimum_required(VERSION 3.28)
project(TowerServer
    VERSION 0.1
    LANGUAGES CXX
)

option(TOWER_BUILD_TESTS "Enable the build of tests" ON)

### DEPENDENCIES ###
# Boost.asio -> Boost.system
# Boost.mysql -> Boost.charconv
find_package(Boost 1.85.0 REQUIRED COMPONENTS charconv system json)

find_package(OpenSSL REQUIRED)

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
    tower/units.hpp

    tower/entity/entity.hpp
    tower/entity/entity_spawner.hpp
    tower/entity/entity_spawner.cpp
    tower/entity/loader.hpp
    tower/entity/loader.cpp
    tower/entity/simple_monster.hpp
    tower/entity/simple_monster.cpp

    tower/game/modifier.hpp
    tower/game/party.hpp

    tower/item/gold.hpp
    tower/item/item.hpp
    tower/item/item_factory.hpp
    tower/item/item_factory.cpp
    tower/item/item_object.hpp
    tower/item/loader.hpp
    tower/item/loader.cpp
    tower/item/stackable.hpp
    tower/item/equipment/equipment.hpp
    tower/item/equipment/fist.hpp
    tower/item/equipment/fist.cpp
    tower/item/equipment/melee_attackable.hpp

    tower/network/client.hpp
    tower/network/client.cpp
    tower/network/connection.hpp
    tower/network/connection.cpp
    tower/network/login_handler.hpp
    tower/network/login_handler.cpp
    tower/network/packet.hpp
    tower/network/profiler.hpp
    tower/network/server.hpp
    tower/network/server.cpp
    tower/network/server_shared_state.hpp
    tower/network/zone.hpp
    tower/network/zone.cpp

    tower/physics/collision_object.hpp
    tower/physics/collision_shape.hpp
    tower/physics/cube_collision_shape.hpp
    tower/physics/cube_collision_shape.cpp
    tower/physics/sphere_collision_shape.hpp
    tower/physics/sphere_collision_shape.cpp

    tower/player/player.hpp
    tower/player/player.cpp
    tower/player/inventory.hpp
    tower/player/inventory.cpp
    tower/player/stat.hpp
    tower/player/state/attacking_state.hpp
    tower/player/state/idle_state.hpp
    tower/player/state/jumping_state.hpp
    tower/player/state/running_state.hpp

    tower/skill/melee_attack.hpp

    tower/system/probe.hpp
    tower/system/random.hpp
    tower/system/settings.hpp
    tower/system/settings.cpp
    tower/system/state.hpp
    tower/system/state_machine.hpp
    tower/system/timer.hpp
    tower/system/container/concurrent_map.hpp
    tower/system/container/concurrent_queue.hpp
    tower/system/container/grid.hpp

    tower/world/world_object.hpp
    tower/world/path_finder.hpp
    tower/world/subworld.cpp
    tower/world/subworld.hpp
)
add_library(tower::libtower ALIAS libtower)

target_compile_features(libtower PUBLIC cxx_std_23)
target_compile_options(libtower PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(libtower
    PUBLIC
    spdlog
    flatbuffers
    glm
    Boost::system

    PRIVATE
    Boost::json
    Boost::charconv
    OpenSSL::SSL
    jwt-cpp::jwt-cpp
)
target_include_directories(libtower PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tower)
target_compile_definitions(libtower PRIVATE
    TOWER_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    TOWER_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

add_executable(tower-server
    tower/main.cpp
)
target_link_libraries(tower-server PRIVATE tower::libtower)


### SCHEMAS ###
file(GLOB_RECURSE packet_schemas schemas/packet/*.fbs)
file(GLOB_RECURSE world_schemas schemas/world/*.fbs)

macro(schema_compiler in_file include_dir out_dir)
    get_filename_component(name ${in_file} NAME_WE)
    set(out_file ${out_dir}/${name}.hpp)

    add_custom_command(
        OUTPUT ${out_file}
        COMMAND
            flatc -I ${include_dir} -o ${out_dir}
            --cpp --cpp-std c++17 --filename-suffix "\"\"" --filename-ext hpp ${in_file}
        DEPENDS ${schema_file} flatc
        COMMENT "[flatc] Compiling schema: ${out_file}"
    )

    add_custom_target(${name}_packet ALL DEPENDS ${out_file})
    add_dependencies(libtower ${name}_packet)

    list(APPEND compiled_schemas ${out_file})
endmacro()

foreach(schema_file ${packet_schemas})
    schema_compiler(${schema_file} ${CMAKE_CURRENT_SOURCE_DIR}/schemas/packet ${CMAKE_CURRENT_BINARY_DIR}/tower/tower/network/packet)
endforeach()

foreach(schema_file ${world_schemas})
    schema_compiler(${schema_file} ${CMAKE_CURRENT_SOURCE_DIR}/schemas/world ${CMAKE_CURRENT_BINARY_DIR}/tower/tower/world/data)
endforeach()


### TESTS ###
if (TOWER_BUILD_TESTS)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.0
    )
    FetchContent_MakeAvailable(Catch2)

    # integration tests
    add_subdirectory(tests)

    # unit tests
    add_executable(tower-unit-tests
        tower/world/path_finder.test.cpp
    )
    target_compile_features(tower-unit-tests PRIVATE cxx_std_23)
    target_link_libraries(tower-unit-tests PRIVATE Catch2::Catch2WithMain tower::libtower)
endif ()
