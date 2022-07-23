# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("gal20")

add_executable(gal20 "src/gal20.cpp")
target_compile_features(gal20 PRIVATE cxx_std_20)
set_target_properties(gal20 PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD_REQUIRED YES)