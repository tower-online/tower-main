cmake_minimum_required(VERSION 3.28)
project(spire
        VERSION 0.1
        LANGUAGES CXX
)

### Dependencies ###
include(FetchContent)

find_package(Boost REQUIRED COMPONENTS system)

#FetchContent_Declare(
#        flatbuffers
#        GIT_REPOSITORY https://github.com/google/flatbuffers.git
#        GIT_TAG v24.3.25
#)
#FetchContent_MakeAvailable(flatbuffers)
#
#FetchContent_Declare(
#        spdlog
#        GIT_REPOSITORY https://github.com/gabime/spdlog.git
#        GIT_TAG v1.14.1
#)
#FetchContent_MakeAvailable(spdlog)
