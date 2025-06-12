
#SDL2
if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
endif()

if(NOT DEFINED SDL2_INCLUDE_DIRS)
    set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/sdl2/include")
endif()

