cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20) # due to std::span
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(QDICE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(QDICE_PARENT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

# Target name can be overridden by parent project (e.g. qd4RamBuild, qd4SagimaW8)
if(NOT DEFINED QD_TARGET_NAME)
    set(QD_TARGET_NAME "qd")
endif()

# Features: pass as list, e.g. set(QD_USE "STD;SDL2;IMGUI") before add_subdirectory
if(NOT DEFINED QD_USE)
    set(QD_USE "STD")
endif()

message(STATUS "[qd] QD_TARGET_NAME = ${QD_TARGET_NAME}")
# Split QD_USE into a list with semicolon (;) as the delimiter
foreach(_feature ${QD_USE})
    string(TOUPPER "${_feature}" _FEATURE_UPPER)
    set(QD_IS_${_FEATURE_UPPER} 1)
    message(STATUS "[qd]   Feature enabled: QD_IS_${_FEATURE_UPPER} = 1")
endforeach()

# Configure defines header in build directory
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qtdDefines.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qtdDefines.h"
    @ONLY
)

set(QDICE_SRC)

function(add_source_group SUBDIR)
    set(CURDIR "${QDICE_DIR}/${SUBDIR}")
    # CONFIGURE_DEPENDS: ensures CMake re-evaluates the glob when source files
    # are added/removed, so incremental builds always pick up changed sources.
    # Without it, file(GLOB) is only evaluated at configure time and new/modified
    # files are silently missed until a full cmake reconfigure.
    file(GLOB SUB_FILES_SRC CONFIGURE_DEPENDS "${CURDIR}/*.cpp" "${CURDIR}/*.h")
    # Add files to local lists
    list(APPEND QDICE_SRC ${SUB_FILES_SRC})
    # Pass updated lists to the global scope
    set(QDICE_SRC ${QDICE_SRC} PARENT_SCOPE)
    # Create a source group files tab in Visual Studio
    source_group("${SUBDIR}" FILES ${SUB_FILES_SRC})
endfunction()

# Use the function to add subdirectories
add_source_group("base")
add_source_group("thread")
add_source_group("stl")
add_source_group("log")
add_source_group("app")
add_source_group("math")
add_source_group("mem")
add_source_group("file")
add_source_group("debug")
add_source_group("enum")
add_source_group("node")
add_source_group("txtNodes")
add_source_group("typeSystem")
if (QD_IS_IMGUI)
    add_source_group("qui")
    add_source_group("qui/comps")
    add_source_group("qui/controls")
    add_source_group("imGui")
    add_source_group("imGui/style")
    if (QD_IS_SDL2)
        add_source_group("imGui/backends/sdl2")
    elseif(WIN32)
        add_source_group("imGui/backends/win32")
    endif()
endif()
add_library(${QD_TARGET_NAME} ${QDICE_HDR} ${QDICE_SRC})


if (MSVC)
    set(QD_NATVIS_FILE "qd.natvis")
    # tranfers natvis file for any target that links to EASTL
    target_sources(${QD_TARGET_NAME} INTERFACE
        $<INSTALL_INTERFACE:${QD_NATVIS_FILE}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${QD_NATVIS_FILE}>
    )
endif()


# Include dirs
target_include_directories(${QD_TARGET_NAME} PUBLIC
    ${QDICE_PARENT}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated qtdDefines.h
)

#dependencies
if(QD_IS_EASTL)
    target_link_libraries(${QD_TARGET_NAME} PRIVATE EASTL)
endif()
if(QD_IS_SDL2)
    target_link_libraries(${QD_TARGET_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
if(QD_IS_IMGUI)
    target_link_libraries(${QD_TARGET_NAME} PRIVATE imgui)
endif()

# Compiler options
if (MSVC)
    #target_compile_options(${QD_TARGET_NAME} PRIVATE /W4 /permissive- /Zc:__cplusplus)

    target_compile_options(${QD_TARGET_NAME} PRIVATE /W4 /WX /bigobj)
    target_link_options(${QD_TARGET_NAME} PRIVATE /INCREMENTAL /SAFESEH:NO)
    target_compile_options(${QD_TARGET_NAME} PRIVATE /MP) # multiprocessor build

endif()

# Append content from the original va_stdafx.h file to the generated file
file(READ "${QDICE_DIR}/va_stdafx.h" VA_STDAFX_CONTENT)
file(WRITE "${PROJECT_BINARY_DIR}/va_stdafx.h" "${VA_STDAFX_CONTENT}")


