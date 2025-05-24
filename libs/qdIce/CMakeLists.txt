cmake_minimum_required(VERSION 3.22)

get_filename_component(QDICE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(QDICE_PARENT ${QDICE_DIR}/.. ABSOLUTE)

set(QDICE_SRC)
set(QDICE_HDR)

function(add_source_group SUBDIR)
    set(CURDIR "${QDICE_DIR}/${SUBDIR}")
    file(GLOB SUB_FILES_CPP "${CURDIR}/*.cpp")
    file(GLOB SUB_FILES_HDR "${CURDIR}/*.h")

    # Add files to local lists
    list(APPEND QDICE_SRC ${SUB_FILES_CPP})
    list(APPEND QDICE_HDR ${SUB_FILES_HDR})

    # Pass updated lists to the global scope
    set(QDICE_SRC ${QDICE_SRC} PARENT_SCOPE)
    set(QDICE_HDR ${QDICE_HDR} PARENT_SCOPE)

    # Create a source group
    source_group("${SUBDIR}" FILES ${SUB_FILES_CPP} ${SUB_FILES_HDR})
endfunction()

# Use the function to add subdirectories
add_source_group("qdBase")
add_source_group("qdThread")
add_source_group("qdGeom")
add_source_group("qdEASTL")
add_source_group("qdLog")
add_source_group("qdImGui")
add_source_group("qdUI")


add_library(qdIce ${QDICE_HDR} ${QDICE_SRC})

# Include dirs
target_include_directories(qdIce PUBLIC ${QDICE_PARENT} ${SDL2_INCLUDE_DIRS})

#dependencies
target_link_libraries(qdIce PRIVATE imgui EASTL ${SDL2_LIBRARIES})
