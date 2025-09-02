cmake_minimum_required(VERSION 3.16)
project(VoxSolution)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

# Engine (DLL)
add_subdirectory(Engine)

# Game (DLL)
add_subdirectory(Game)

# Launcher
add_executable(VoxCraft EntryPoint.cpp)
target_link_libraries(VoxCraft PRIVATE VoxCraftDLL)
set_target_properties(VoxCraft PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
)

