

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
