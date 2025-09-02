cmake_minimum_required(VERSION 3.16)
project(VoxCraftDLL)

set(CMAKE_CXX_STANDARD 23)


add_library(VoxCraftDLL SHARED GameEntry.cpp)
target_link_libraries(VoxCraftDLL PRIVATE VoxCore)

set_target_properties(VoxCraftDLL PROPERTIES
        PREFIX ""
        OUTPUT_NAME "VoxCraft"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Game/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Game/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Game/bin
)
