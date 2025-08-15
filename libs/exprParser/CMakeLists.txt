# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

project(exprParser)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(exprParser STATIC
    parser/common.cpp
    parser/common.h
    parser/lexer.cpp
    parser/lexer.h
    parser/parser_oop.cpp
    parser/parser_oop.h
    parser/resolve_oop.h
    )

# Include dirs
target_include_directories(exprParser PUBLIC .)