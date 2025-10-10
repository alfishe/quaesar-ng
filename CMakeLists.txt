cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(quaesar LANGUAGES CXX)

#set(CMAKE_GENERATOR_TOOLSET "ClangCl") # test compilation on clang-c
option(ENABLE_CODE_GENERATION "Enable code generation" OFF)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(WARNING "You are doing an in-source build. Recommended:
      ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B build
      ${CMAKE_COMMAND} --build build")
endif()


# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# Zlib and disable examples

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CAPSTONE_M68K_SUPPORT ON CACHE BOOL "" FORCE)


# Include CMake helper utils
include(scripts/cmake/compile_options.cmake)
include(scripts/cmake/format_sources.cmake)
include(external/sdl2/sdl2.cmake)


# ---------------------------------------------------------------------------
# Global list of libraries to link with 'quaesar' collected from child cmake files
# Children can call: quaesar_add_libs(<lib1> <lib2> ...)
# ---------------------------------------------------------------------------
set(QUAESAR_EXTRA_LIBS "" CACHE INTERNAL "Accumulated extra libraries for quaesar final link")
function(quaesar_add_libs)
    foreach(lib IN LISTS ARGN)
        if(lib)
            list(APPEND QUAESAR_EXTRA_LIBS "${lib}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES QUAESAR_EXTRA_LIBS)
    # Write back to cache so subdirectories (different scopes) see updated value
    set(QUAESAR_EXTRA_LIBS "${QUAESAR_EXTRA_LIBS}" CACHE INTERNAL "Accumulated extra libraries for quaesar final link" FORCE)
endfunction()



# Include CMakelists.txt libraries
set(UAE_CUSTOM_IMPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/uae_lib_imp" CACHE INTERNAL "Path to UAE custom implementation directory")
add_subdirectory(libs/uae_lib)
add_subdirectory(external/zlib)
add_subdirectory(external/ADFlib)
add_subdirectory(external/dear_imgui)
add_subdirectory(external/EASTL)
add_subdirectory(external/capstone)
add_subdirectory(external/nativefiledialog-extended) #nfd
add_subdirectory(libs/qd)
add_subdirectory(libs/amDebugger)
add_subdirectory(libs/exprParser)

option(VAMIGA "Enable VAmiga support" ON)
if (VAMIGA)
    include(VAmigaLib.cmake)
endif()
#-------------------------------------------------------------------------------------------

# Quasar *.h/cpp files
file(GLOB_RECURSE QUAESAR_SOURCES "src/*.cpp")
file(GLOB_RECURSE QUAESAR_HEADERS "src/*.h")

#-------------------------------------------------------------------------------
# Run CMake converter to put images  into the data segment
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")
# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")
# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=resources_inc.cpp -DOUTPUT_H=resources_inc.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/bin2c.cmake")

# Run bin2c immediately at configure time
message(STATUS "Running bin2c (configure-time)...('${CMAKE_COMMAND} ${bin2c_cmdline}')")
execute_process(
    COMMAND ${CMAKE_COMMAND} ${bin2c_cmdline}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE bin2c_result
    OUTPUT_VARIABLE bin2c_stdout
    ERROR_VARIABLE bin2c_stderr
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)
if (NOT bin2c_result EQUAL 0)
    message(FATAL_ERROR "bin2c failed (exit ${bin2c_result})\nstdout:\n${bin2c_stdout}\nstderr:\n${bin2c_stderr}")
endif()

# Create a custom target for nanogui_resources
add_custom_target(incbin_resources DEPENDS resources_inc.cpp resources_inc.h)

# Needed to generated files
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Recursively glob for all .cpp files in the src directory
list(APPEND QUAESAR_SOURCES resources_inc.cpp)
list(APPEND QUAESAR_HEADERS resources_inc.h)
#-------------------------------------------------------------------------------------------

# Create VisualStudio groups tree in solution with root at CMAKE_CURRENT_SOURCE_DIR.
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
    target_compile_definitions(quaesar PUBLIC MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0)
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


include(user_custoom_plugins.cmake OPTIONAL)

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
    zlibstatic
    ${SDL2_LIBRARIES}
    EASTL
    imgui
    nfd
    qd
    uae_lib
    amDebugger
    ${QUAESAR_EXTRA_LIBS}
)


#if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
# file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/bin/install/default_layout.ini" "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
#endif()


add_option_edit_and_continue(quaesar)
add_option_edit_and_continue(qd)
add_option_edit_and_continue(amDebugger)


if (WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar) # makes startup project
    add_dependencies(quaesar quaesar-clang-format)
endif()

