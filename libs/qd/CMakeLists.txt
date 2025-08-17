cmake_minimum_required(VERSION 3.15)

get_filename_component(QDICE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(QDICE_PARENT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

set(QDICE_SRC)

function(add_source_group SUBDIR)
    set(CURDIR "${QDICE_DIR}/${SUBDIR}")
    file(GLOB SUB_FILES_SRC "${CURDIR}/*.cpp" "${CURDIR}/*.h")
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
add_source_group("net")
add_source_group("debug")
add_source_group("enum")
add_source_group("platform")
add_source_group("node")
add_source_group("typeSystem")
add_source_group("qui")
add_source_group("qui/comps")
add_source_group("qui/controls")
add_source_group("qimGui")
add_source_group("qimGui/controls")
add_source_group("imGui")
add_source_group("imGui/style")
add_source_group("imGui/backends")
if (MSVC)
list(APPEND QDICE_SRC "qd.natvis")
endif()

add_library(qd ${QDICE_HDR} ${QDICE_SRC})

# Include dirs
target_include_directories(qd PUBLIC ${QDICE_PARENT} ${SDL2_INCLUDE_DIRS})

#dependencies
target_link_libraries(qd PRIVATE imgui EASTL ${SDL2_LIBRARIES})


# Append content from the original va_stdafx.h file to the generated file
file(READ "${QDICE_DIR}/va_stdafx.h" VA_STDAFX_CONTENT)
file(WRITE "${PROJECT_BINARY_DIR}/va_stdafx.h" "${VA_STDAFX_CONTENT}")

