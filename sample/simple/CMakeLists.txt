cmake_minimum_required(VERSION 3.26)

add_executable(simple main.cpp)

target_link_libraries(simple PRIVATE warnings gal20_lib)

target_compile_features(simple PRIVATE cxx_std_20)
set_target_properties(simple PROPERTIES
	CXX_EXTENSIONS NO
	CXX_STANDARD_REQUIRED YES)
