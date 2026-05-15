cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(amDebugger LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

get_filename_component(AM_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(AM_SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src ABSOLUTE)

set(AM_SRC)

function(add_source_group SUBDIR)
    set(CURDIR "${AM_SRC_INCLUDE_DIR}/amDebugger/${SUBDIR}")
    file(GLOB SUB_FILES_SRC "${CURDIR}/*.cpp" "${CURDIR}/*.h")
    # Add files to local lists
    list(APPEND AM_SRC ${SUB_FILES_SRC})
    # Pass updated lists to the global scope
    set(AM_SRC ${AM_SRC} PARENT_SCOPE)
    # Create a source group
    source_group("${SUBDIR}" FILES ${SUB_FILES_SRC})
endfunction()

# Use the function to add subdirectories
add_source_group("")
add_source_group("ui")
add_source_group("vm")
add_source_group("window")
add_source_group("emuDriv")
add_source_group("codeAnalyzer")


add_library(${PROJECT_NAME} ${AM_SRC})

#Whole archive option is needed to ensure that all windows are linked in the final binary
# (debugger windows use static-initializer-based auto-registration via TS_BEGIN_REFLECT_CLASS)
if(MSVC)
target_link_options(${PROJECT_NAME} INTERFACE "/WHOLEARCHIVE:${PROJECT_NAME}")
endif()

# Include dirs
target_include_directories(${PROJECT_NAME} PUBLIC ${AM_SRC_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})

#dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE qd imgui EASTL exprParser capstone)
