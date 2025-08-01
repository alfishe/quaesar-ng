# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

#set(CMAKE_GENERATOR_TOOLSET "ClangCl") # test compilation on clang-c

option(ENABLE_CODE_GENERATION "Enable code generation" OFF)

# Project name and languages used
project(quaesar LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zlib and disable examples

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CAPSTONE_M68K_SUPPORT ON CACHE BOOL "" FORCE)

include(scripts/cmake/format_sources.cmake)
include(scripts/cmake/compile_options.cmake)
include(external/sdl2/sdl2.cmake)

add_subdirectory(external/zlib)
add_subdirectory(external/ADFlib)
add_subdirectory(external/dear_imgui)
add_subdirectory(external/EASTL)
add_subdirectory(external/capstone)
add_subdirectory(external/nativefiledialog)
add_subdirectory(libs/qd)
add_subdirectory(libs/amDebugger)
add_subdirectory(libs/exprParser)


# Define a global variable
set(UAE_CUSTOM_IMPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/uae_lib_imp" CACHE INTERNAL "Path to UAE custom implementation directory")
add_subdirectory(libs/uae_lib)

# Quasar *.h/cpp files
file(GLOB_RECURSE QUAESAR_SOURCES "src/*.cpp")
file(GLOB_RECURSE QUAESAR_HEADERS "src/*.h")

#-------------------------------------------------------------------------------
# Run simple cmake converter to put font files into the data segment
# Glob up resource files
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")

# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=resources_inc.cpp
  -DOUTPUT_H=resources_inc.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/bin2c.cmake")

# Run bin2c on resource files
add_custom_command(
  OUTPUT resources_inc.cpp resources_inc.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  VERBATIM)

# Create a custom target for nanogui_resources
add_custom_target(incbin_resources DEPENDS resources_inc.cpp resources_inc.h)

# Needed to generated files
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Recursively glob for all .cpp files in the src directory
list(APPEND QUAESAR_SOURCES resources_inc.cpp)
list(APPEND QUAESAR_HEADERS resources_inc.h)
#-------------------------------------------------------------------------------------------

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${QUAESAR_HEADERS} ${QUAESAR_SOURCES})

#-------------------------------------------------------------------------------------------
add_executable(quaesar ${QUAESAR_SOURCES})
target_sources(quaesar PRIVATE ${QUAESAR_SOURCES} ${QUAESAR_HEADERS})
add_dependencies(quaesar incbin_resources)
set_target_properties(quaesar PROPERTIES
    OUTPUT_NAME_DEBUG "quaesar-dbg"
    OUTPUT_NAME_RELEASE "quaesar"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
)
if (WIN32)
    set_target_properties(quaesar PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()


 # Link libraries
if (APPLE OR LINUX OR UNIX)
	target_compile_options(quaesar PRIVATE -DUAE=1 -D_cdecl= -DOS_NAME=\"linux\")
    target_compile_definitions(quaesar PRIVATE FSUAE)
endif()

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/sdl2.lib")
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/SDL2main.lib")
    else()
       target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/sdl2.lib")
	   target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/SDL2main.lib")
    endif()
    target_link_libraries(quaesar PRIVATE Ws2_32.lib Winmm.lib Version.lib Imm32.lib Setupapi.lib)
	set_target_properties(quaesar PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

if (APPLE)
    target_compile_options(quaesar PRIVATE -Wno-macro-redefined -Wno-deprecated-declarations)
endif()

if (LINUX OR UNIX)
    target_link_libraries(quaesar PRIVATE dl)
endif()

#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(quaesar PRIVATE
    ${SDL2_INCLUDE_DIRS}
	external/ADFlib/src
	external
	libs
	src
    src/quasar_app
	src/uae_lib_imp
)

target_link_libraries(quaesar PRIVATE
    adf
    uae_lib
    zlibstatic
    ${SDL2_LIBRARIES}
    imgui
    EASTL
    qd
    amDebugger
    nfd
)


#if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
    #file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/bin/install/default_layout.ini" "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
#endif()


add_option_edit_and_continue(quaesar)
add_option_edit_and_continue(qd)
add_option_edit_and_continue(amDebugger)


if (WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar) # makes startup project
    add_dependencies(quaesar quaesar-clang-format)
endif()

