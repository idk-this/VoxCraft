cmake_minimum_required(VERSION 3.16)
project(VoxSolution)

set(CMAKE_CXX_STANDARD 23)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
endforeach()

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
endif()

# Engine (DLL)
add_subdirectory(Engine)

# Game (DLL)
add_subdirectory(Game)

# Launcher
add_executable(VoxCraft EntryPoint.cpp)
if(MINGW)
    target_compile_options(VoxCraft PRIVATE -static-libgcc -static-libstdc++ -lpthread --static)
    target_link_options(VoxCraft PRIVATE -static-libgcc -static-libstdc++ -lpthread --static)
endif()
target_link_libraries(VoxCraft PRIVATE VoxCraftGame)

set_target_properties(VoxCraft PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)
