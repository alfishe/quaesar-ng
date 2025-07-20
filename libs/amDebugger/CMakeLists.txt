cmake_minimum_required(VERSION 3.22)

project(amDebugger LANGUAGES CXX)

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
add_source_group("operation")
add_source_group("shortcut")
add_source_group("ui")
add_source_group("vm")
add_source_group("window")
add_source_group("emuDrivers")


add_library(${PROJECT_NAME} ${AM_SRC})

#Whole archive option is needed to ensure that all windows are linked in the final binary
target_link_options(${PROJECT_NAME} INTERFACE "/WHOLEARCHIVE:${PROJECT_NAME}")

# Include dirs
target_include_directories(${PROJECT_NAME} PUBLIC ${AM_SRC_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})

#dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE qd imgui EASTL exprParser capstone)
