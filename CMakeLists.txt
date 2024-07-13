cmake_minimum_required(VERSION 3.28)
project(spire-server
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


### TARGETS ###
add_executable(spire-server
    spire/main.cpp

    spire/containers/concurrent_queue.hpp

    spire/game/client.hpp
    spire/game/client.cpp

    spire/net/connection.hpp
    spire/net/listener.hpp
)
target_compile_features(spire-server PRIVATE cxx_std_20)
target_link_libraries(spire-server PRIVATE Boost::system spdlog flatbuffers)
target_include_directories(spire-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB schemas RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} spire/net/packet/schemas/*.fbs)
add_custom_target(spire-packet-schemas-compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND flatc --cpp --cpp-std c++17 --filename-suffix "\"\"" --filename-ext hpp -o spire/net/packet/ ${schemas}
    DEPENDS flatc
)