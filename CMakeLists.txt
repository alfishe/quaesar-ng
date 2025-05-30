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

add_subdirectory(external/zlib)
add_subdirectory(external/ADFlib)
add_subdirectory(external/dear_imgui)
add_subdirectory(external/EASTL)
add_subdirectory(external/capstone)

#SDL2
if (APPLE OR LINUX OR UNIX)
    find_package(SDL2 REQUIRED)
endif()

if(NOT DEFINED SDL2_INCLUDE_DIRS)
    set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/sdl2/include")
endif()

add_subdirectory(libs/qdIce)
add_subdirectory(libs/amDebugger)


include_directories(uae_src/include uae_src src .)

# Include the source files of the UAE
include("${CMAKE_SOURCE_DIR}/uae_src.cmake")
add_executable(quaesar ${UAE_SOURCE_LIST})


if (WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar) # makes startup project
    add_dependencies(quaesar quaesar-clang-format)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UAE_SOURCE_LIST})

# Recursively glob for all .cpp files in the src directory
file(GLOB_RECURSE CROSS_FILES "src/*.cpp")
file(GLOB_RECURSE QUAE_HEADERS "src/*.h")
target_sources(quaesar PRIVATE ${CROSS_FILES} ${QUAE_HEADERS})

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${QUAE_HEADERS} ${CROSS_FILES} ${SDL2_INCLUDE_DIRS})

target_include_directories(quaesar PRIVATE ${SDL2_INCLUDE_DIRS})

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

target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/ADFlib/src")
target_include_directories(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external")
target_link_libraries(quaesar PRIVATE ${SDL2_LIBRARIES} zlibstatic adf imgui EASTL qdIce amDebugger)


if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
    file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/bin/install/default_layout.ini" "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
endif()


# Format source code with clang-format
if(WIN32)
    set(CLANG_FORMAT_EXE "${CMAKE_SOURCE_DIR}/bin/windows/bin/clang-format.exe")
else()
    find_program(CLANG_FORMAT_EXE NAMES clang-format)
endif()


if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE QUAE_FORMAT_SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
    )

    # Create a stamp file to track formatting
    set(CLANG_FORMAT_STAMP "${CMAKE_BINARY_DIR}/.clang_format.stamp")

    add_custom_command(
        OUTPUT ${CLANG_FORMAT_STAMP}
        COMMAND ${CMAKE_COMMAND} -E echo "Format sources with clang-format: ${CLANG_FORMAT_EXE}"
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${QUAE_FORMAT_SRC_FILES}
        COMMAND ${CMAKE_COMMAND} -E touch ${CLANG_FORMAT_STAMP}
        DEPENDS ${QUAE_FORMAT_SRC_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on changed files"
    )

    add_custom_target(
        quaesar-clang-format
        DEPENDS ${CLANG_FORMAT_STAMP}
    )
endif()


# Function to add Edit and Continue support for MSVC
function(add_option_edit_and_continue target_name)
    if (WIN32)
        add_definitions(-D_HAS_STD_BYTE=0)
        if(MSVC) # Enable Edit and Continue for Debug builds
            target_compile_options(${target_name} PRIVATE "/ZI")
            #string(REGEX REPLACE "/Z[iI7]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
            #target_compile_options(${target_name} PRIVATE ${CMAKE_CXX_FLAGS_DEBUG})

            target_link_options(${target_name} PRIVATE "/INCREMENTAL")
            target_compile_options(${target_name} PRIVATE "/MP") # multiprocessor build
            
            if(NOT CMAKE_GENERATOR_PLATFORM)
                set(CMAKE_GENERATOR_PLATFORM "Win32") # add Win32 platform to solution
            endif()
        endif()
    endif()
endfunction()

add_option_edit_and_continue(quaesar)
add_option_edit_and_continue(qdIce)
add_option_edit_and_continue(amDebugger)
