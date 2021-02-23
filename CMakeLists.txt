cmake_minimum_required(VERSION 3.19)
project(util_parseFlaticon)

set (CMAKE_AUTOMOC ON)
set (CMAKE_CXX_STANDARD 20)

find_package(Qt6 COMPONENTS Core)

add_executable(util_parseFlaticon main.cpp)

target_link_libraries(util_parseFlaticon Qt6::Core)

add_definitions( -DMY_RC_PATH="${CMAKE_CURRENT_SOURCE_DIR}" )
