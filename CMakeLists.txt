# Корневой CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(VoxSolution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка выходных директорий
foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${OUTPUTCONFIG}" OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
endforeach()

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
endif()

# Подпроекты
add_subdirectory(Engine)
add_subdirectory(Game)

# Исполняемый файл
add_executable(VoxCraft EntryPoint.cpp)

if(MINGW)
    target_link_options(VoxCraft PRIVATE -static -static-libgcc -static-libstdc++ -lpthread)
endif()

target_link_libraries(VoxCraft PRIVATE VoxCraftGame)

# Настройка выходных директорий для исполняемого файла
set_target_properties(VoxCraft PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)