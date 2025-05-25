cmake_minimum_required(VERSION 3.22)

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
    # Create a source group
    source_group("${SUBDIR}" FILES ${SUB_FILES_SRC})
endfunction()

# Use the function to add subdirectories
add_source_group("qdBase")
add_source_group("qdThread")
add_source_group("qdSTL")
add_source_group("qdLog")
add_source_group("qdImGui")
add_source_group("qdUI")
add_source_group("qdApp")
add_source_group("qdMath")
add_source_group("qdMem")
add_source_group("qdDebug")
add_source_group("qdEnum")
add_source_group("qdPlatform")

add_library(qdIce ${QDICE_HDR} ${QDICE_SRC})

# Include dirs
target_include_directories(qdIce PUBLIC ${QDICE_PARENT} ${SDL2_INCLUDE_DIRS})

#dependencies
target_link_libraries(qdIce PRIVATE imgui EASTL ${SDL2_LIBRARIES})
