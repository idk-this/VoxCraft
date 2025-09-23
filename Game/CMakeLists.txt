# Game/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(VoxCraftGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE GAME_SRC
        Application/*.cpp
        Application/*.h
        ECS/*.h
        ECS/*.cpp
        ECS/**/*.h
        ECS/**/*.cpp
)
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(UNIX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

set(RESOURCES_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
set(RESOURCES_DST "${CMAKE_BINARY_DIR}/$<CONFIG>/Content")

set(LIBS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/runtime_libs")
set(LIBS_DST "${CMAKE_BINARY_DIR}/$<CONFIG>/Game/lib")


add_library(VoxCraftGame SHARED ${GAME_SRC} GameEntry.cpp)
target_include_directories(VoxCraftGame PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(TARGET VoxCraftGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RESOURCES_DST}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${RESOURCES_SRC}" "${RESOURCES_DST}"
        COMMENT "Copying resources to ${RESOURCES_DST}"
)

if (EXISTS "${LIBS_SRC}" AND IS_DIRECTORY "${LIBS_SRC}")
    file(GLOB LIB_FILES "${LIBS_SRC}/*")
    if (LIB_FILES)
        add_custom_command(TARGET VoxCraftGame POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBS_DST}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${LIBS_SRC}" "${LIBS_DST}"
                COMMENT "Copying libs to ${LIBS_DST}"
        )
    endif()
endif()

target_link_libraries(VoxCraftGame PRIVATE VoxCore)

set_target_properties(VoxCraftGame PROPERTIES
        PREFIX ""
        OUTPUT_NAME "VoxCraft.Game"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/Game"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/Game"
)