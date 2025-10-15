# Game/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(VoxCraftGame VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VOXCRAFT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VOXCRAFT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VOXCRAFT_VERSION_PATCH ${PROJECT_VERSION_PATCH})

string(TIMESTAMP VOXCRAFT_BUILD_DATE "%Y-%m-%d %H:%M:%S")

set(VOXCRAFT_BUILD_NUMBER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build_number.txt")
if(EXISTS ${VOXCRAFT_BUILD_NUMBER_FILE})
    file(READ ${VOXCRAFT_BUILD_NUMBER_FILE} VOXCRAFT_BUILD_NUMBER)
    string(STRIP ${VOXCRAFT_BUILD_NUMBER} VOXCRAFT_BUILD_NUMBER)
else()
    set(VOXCRAFT_BUILD_NUMBER 0)
endif()
math(EXPR VOXCRAFT_BUILD_NUMBER "${VOXCRAFT_BUILD_NUMBER} + 1")

file(WRITE ${VOXCRAFT_BUILD_NUMBER_FILE} "${VOXCRAFT_BUILD_NUMBER}")

file(GLOB_RECURSE GAME_SRC
        Application/*.cpp
        Application/*.h
        ECS/*.h
        ECS/*.cpp
        ECS/**/*.h
        ECS/**/*.cpp
        UI/*.h
        UI/*.cpp
        UI/**/*.h
        UI/**/*.cpp

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

target_compile_definitions(VoxCraftGame PUBLIC
        VOXCRAFT_VERSION_MAJOR=${VOXCRAFT_VERSION_MAJOR}
        VOXCRAFT_VERSION_MINOR=${VOXCRAFT_VERSION_MINOR}
        VOXCRAFT_VERSION_PATCH=${VOXCRAFT_VERSION_PATCH}
        VOXCRAFT_BUILD_NUMBER=${VOXCRAFT_BUILD_NUMBER}
        VOXCRAFT_BUILD_DATE="${VOXCRAFT_BUILD_DATE}"
        VOXCRAFT_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
        VOXCRAFT_VERSION_STR="${VOXCRAFT_VERSION_MAJOR}.${VOXCRAFT_VERSION_MINOR}.${VOXCRAFT_VERSION_PATCH}"
        VOXCRAFT_VERSION_FULL_STR="${VOXCRAFT_VERSION_MAJOR}.${VOXCRAFT_VERSION_MINOR}.${VOXCRAFT_VERSION_PATCH}.${VOXCRAFT_BUILD_NUMBER}"
)
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

target_link_libraries(VoxCraftGame PRIVATE VoxCore
        imgui
        nlohmann_json::nlohmann_json
)

set_target_properties(VoxCraftGame PROPERTIES
        PREFIX ""
        OUTPUT_NAME "VoxCraft.Game"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/Game"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/Game"
)