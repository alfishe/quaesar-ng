
#SDL2
if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
elseif(MINGW)
    find_package(SDL2 REQUIRED)
    add_definitions(-DSDL_MAIN_HANDLED)
    # Set SDL2_LIBRARIES to the proper target for libs that use it
    set(SDL2_LIBRARIES SDL2::SDL2)
endif()

if(NOT DEFINED SDL2_INCLUDE_DIRS)
    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
endif()

