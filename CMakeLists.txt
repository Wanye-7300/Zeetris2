cmake_minimum_required(VERSION 3.30)
project(Zeetris2)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(Boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.87.0
        GIT_SHALLOW ON)
FetchContent_MakeAvailable(Boost)

add_executable(Zeetris2 main.cpp
        game.cpp
        game.h)
target_link_libraries(Zeetris2 PRIVATE SFML::Graphics)
target_link_libraries(Zeetris2 PRIVATE Boost::asio Boost::bind)
if (MSVC)
    target_compile_options(Zeetris2 PRIVATE /W4)
endif ()

add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")
