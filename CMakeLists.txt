cmake_minimum_required(VERSION 3.28)
project(spire-server
    VERSION 0.1
    LANGUAGES CXX
)

### DEPENDENCIES ###
include(FetchContent)

find_package(Boost REQUIRED COMPONENTS system)

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
add_library(net INTERFACE
    spire/net/connection.hpp
    spire/net/listener.hpp
)
add_library(spire::net ALIAS net)
target_compile_features(net INTERFACE cxx_std_20)
target_include_directories(net INTERFACE server)
target_link_libraries(net INTERFACE Boost::system spdlog flatbuffers)

add_executable(spire-server spire/main.cpp)
target_compile_features(spire-server PRIVATE cxx_std_20)
target_link_libraries(spire-server PRIVATE spire::net)