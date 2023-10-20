cmake_minimum_required (VERSION 3.26)

project (gal20 CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(src bin)
add_subdirectory(sample)

include(Install)

