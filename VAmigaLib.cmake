cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(VAMIGA ON CACHE BOOL "Enable VAmiga support")

add_subdirectory(libs/vAmiga)
add_subdirectory(libs/vAmiga_imp_lib)

quaesar_add_libs(VAmigaImpLib)
