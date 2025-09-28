cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Create VAmiga Implementation Library
file(GLOB_RECURSE VAMIGA_IMP_LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c*"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h*"
)

add_library(VamigaImpLib STATIC
    ${VAMIGA_IMP_LIB_SOURCES}
)
# Create Visual Studio source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VAMIGA_IMP_LIB_SOURCES})

# Set target properties
set_target_properties(VamigaImpLib PROPERTIES
    OUTPUT_NAME "VamigaImpLib"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED True
    CXX_EXTENSIONS False
)


# Include directories for the library
target_include_directories(VamigaImpLib PUBLIC
     ${CMAKE_SOURCE_DIR}/src
#     ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link required libraries
target_link_libraries(VamigaImpLib PRIVATE
    EASTL
    qd
    amDebugger
    imgui
    VACore
)
#include(../../scripts/cmake/compile_options.cmake)
add_option_edit_and_continue(VACore)
add_option_edit_and_continue(VamigaImpLib)


