# CMakeList.txt : CMake project for gal20, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (gal20 "gal20.cpp" "gal20.h")


target_compile_features(gal20 PUBLIC cxx_std_20)
set_target_properties(gal20 PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD_REQUIRED YES)

# TODO: Add tests and install targets if needed.
