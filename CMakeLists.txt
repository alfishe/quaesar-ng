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


# Ставим статическую CRT для всех конфигураций (Debug -> /MTd, Release/RelWithDebInfo/MinSizeRel -> /MT)
if (MSVC)
    # Включаем опцию чтобы можно было отключить через -DUSE_STATIC_MSVC_RUNTIME=OFF
    option(USE_STATIC_MSVC_RUNTIME "Link with static MSVC runtime (/MT,/MTd)" ON)
    if (USE_STATIC_MSVC_RUNTIME)
        # Генераторное выражение добавляет 'Debug' суффикс только для Debug сборки
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

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
add_subdirectory(libs/amDebugger)
add_subdirectory(libs/exprParser)

set(QD_USE "EASTL;SDL2;IMGUI")
add_subdirectory(libs/qd)

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

# Run bin2c at configure time, but only if resources changed (or files were added/removed/renamed).
# This prevents full rebuilds on every CMake re-run (triggered by CONFIGURE_DEPENDS)
# by checking:
# 1. Whether the list of resources has changed.
# 2. Whether the newest resource is newer than the generated output.
set(_resources_changed FALSE)
set(_newest_resource 0)
foreach(_res ${resources})
    get_filename_component(_res_abs "${_res}" ABSOLUTE)
    file(TIMESTAMP "${_res_abs}" _res_mtime "%s")
    if(_res_mtime GREATER _newest_resource)
        set(_newest_resource ${_res_mtime})
    endif()
endforeach()

set(_resources_list_file "${CMAKE_CURRENT_BINARY_DIR}/resources_list.txt")
set(_resources_list_changed FALSE)
if(EXISTS "${_resources_list_file}")
    file(READ "${_resources_list_file}" _old_resources_list)
    if(NOT "${_old_resources_list}" STREQUAL "${resources}")
        set(_resources_list_changed TRUE)
    endif()
else()
    set(_resources_list_changed TRUE)
endif()

set(_output_c "${CMAKE_CURRENT_BINARY_DIR}/resources_inc.cpp")
if(EXISTS "${_output_c}")
    file(TIMESTAMP "${_output_c}" _out_mtime "%s")
    if(_newest_resource GREATER _out_mtime)
        set(_resources_changed TRUE)
    endif()
else()
    set(_resources_changed TRUE)
endif()

if(_resources_changed OR _resources_list_changed)
    message(STATUS "Running bin2c (resources changed)...")
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
    file(WRITE "${_resources_list_file}" "${resources}")
else()
    message(STATUS "bin2c: resources unchanged, skipping regeneration")
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

if (MSVC AND CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Debug;Release;MinSizeRel" CACHE STRING "" FORCE)
endif()

 # Link libraries
if (APPLE OR LINUX OR UNIX)
	target_compile_options(quaesar PRIVATE -DUAE=1 -D_cdecl= -DOS_NAME=\"linux\")
    target_compile_definitions(quaesar PRIVATE FSUAE)
endif()

if (WIN32)
    if (MINGW)
        # MinGW: use system SDL2 via find_package
        find_package(SDL2 REQUIRED)
        add_definitions(-DSDL_MAIN_HANDLED)
        target_link_libraries(quaesar PRIVATE SDL2::SDL2)
        target_link_libraries(quaesar PRIVATE ws2_32 winmm version imm32 setupapi dbghelp)
    else()
        # MSVC: use bundled SDL2 libraries
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
           target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/sdl2.lib")
           target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/x64/SDL2main.lib")
        else()
           target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/sdl2.lib")
           target_link_libraries(quaesar PRIVATE "${CMAKE_SOURCE_DIR}/external/sdl2/win32/SDL2main.lib")
        endif()
        target_link_libraries(quaesar PRIVATE Ws2_32.lib Winmm.lib Version.lib Imm32.lib Setupapi.lib dbghelp.lib)
        set_target_properties(quaesar PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
    endif()
endif()

if (APPLE)
    target_compile_options(quaesar PRIVATE -Wno-macro-redefined -Wno-deprecated-declarations)
    # Silence macOS ld informational "ignoring duplicate libraries" for static libs linked multiple times
    target_link_options(quaesar PRIVATE "-Wl,-no_warn_duplicate_libraries")
endif()

if (LINUX OR UNIX)
    target_link_libraries(quaesar PRIVATE dl)
endif()


# Optional plugins dev
include(user_custom_plugins.cmake OPTIONAL)

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

# Force-load the entire amDebugger static library so that auto-registered debugger
# windows (TS_BEGIN_REFLECT_CLASS static initializers) are not stripped by the linker.
# MSVC uses WHOLEARCHIVE (set in amDebugger CMakeLists.txt INTERFACE option).
# macOS uses -force_load; Linux uses --whole-archive.
if(APPLE)
    target_link_options(quaesar PRIVATE "-Wl,-force_load,$<TARGET_FILE:amDebugger>")
elseif(LINUX OR UNIX)
    target_link_options(quaesar PRIVATE
        "-Wl,--whole-archive" "$<TARGET_FILE:amDebugger>" "-Wl,--no-whole-archive")
endif()

# Same problem for VAmigaImpLib: its VAmigaServerProviderFactory registers via a
# static RegOnLoadAppPartServerFactory initializer. On Windows this is handled by
# /WHOLEARCHIVE in libs/vAmiga_imp_lib/CMakeLists.txt. On macOS/Linux the linker
# strips the TU unless force-loaded, so --engine vamiga finds no factory.
if(TARGET VAmigaImpLib)
    if(APPLE)
        target_link_options(quaesar PRIVATE "-Wl,-force_load,$<TARGET_FILE:VAmigaImpLib>")
    elseif(LINUX OR UNIX)
        target_link_options(quaesar PRIVATE
            "-Wl,--whole-archive" "$<TARGET_FILE:VAmigaImpLib>" "-Wl,--no-whole-archive")
    endif()
endif()


#if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
# file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/bin/install/default_layout.ini" "${CMAKE_CURRENT_BINARY_DIR}/imgui.ini")
#endif()

# Bundle the bundled default debugger dock layout next to the binary.
# DebuggerApp::initImGui() resolves it via SDL_GetBasePath() (the dir of the running executable)
# and loads it on first launch when no user imgui.ini exists, so the debugger window
# does not come up as an empty (red) dockspace.
# Using a POST_BUILD custom command + $<TARGET_FILE_DIR:quaesar> ensures the layout
# lands next to the actual exe on every platform/generator (incl. multi-config VS/Xcode).
add_custom_command(TARGET quaesar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/resources/default_layout.ini"
        "$<TARGET_FILE_DIR:quaesar>/default_layout.ini"
    VERBATIM
)


add_option_edit_and_continue(quaesar)
add_option_edit_and_continue(qd)
add_option_edit_and_continue(amDebugger)


if (WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT quaesar) # makes startup project
    add_dependencies(quaesar quaesar-clang-format)
endif()

