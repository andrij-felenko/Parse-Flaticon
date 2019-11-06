project(util_parseFlaticon)

find_package(Qt5Core)
add_executable(util_parseFlaticon main.cpp)
target_link_libraries(util_parseFlaticon Qt5::Core)
add_definitions( -DMY_RC_PATH="${CMAKE_CURRENT_SOURCE_DIR}" )
