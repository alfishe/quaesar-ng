cmake_minimum_required(VERSION 3.10)
project(imgui)

if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
endif()

# Specify C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

get_filename_component(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui ABSOLUTE)
get_filename_component(IMGUI_PARENT ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# Add ImGui source files
file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp)
#list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp)
#list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp)
if (MSVC)
list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/misc/debuggers/imgui.natvis)
endif()

# Include directories for ImGui (SDL2 include paths should be handled in the main CMakeLists.txt)
#include_directories(${IMGUI_DIR}/backends)

# Create static library
add_library(imgui STATIC ${IMGUI_SOURCES})

if (APPLE OR LINUX OR UNIX)
    target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(imgui PRIVATE ${SDL2_LIBRARIES})
else()
    target_include_directories(imgui PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/include")
endif()

# Include dirs
target_include_directories(imgui PUBLIC ${IMGUI_PARENT})
