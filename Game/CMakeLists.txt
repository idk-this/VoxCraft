cmake_minimum_required(VERSION 3.16)
project(VoxCraftGame)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE GAME_SRC
        Application/*.cpp
)
file(GLOB_RECURSE GAME_HDR
        Application/*.h
)

add_library(VoxCraftGame SHARED  ${GAME_SRC} ${GAME_HDR}
GameEntry.cpp)


target_link_libraries(VoxCraftGame PRIVATE VoxCore)


set_target_properties(VoxCraftGame PROPERTIES
        PREFIX ""
        OUTPUT_NAME "VoxCraft"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Game/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Game/bin
)
