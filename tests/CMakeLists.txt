cmake_minimum_required(VERSION 3.26)

find_package(GTest CONFIG REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(tests
  multiobjective/pareto_tests.cpp
  multiobjective/ranking_tests.cpp
  multiobjective/elitism_tests.cpp
  multiobjective/clustering_tests.cpp
  multiobjective/crowding_tests.cpp
  multiobjective/pruning_tests.cpp
  multiobjective/projection_tests.cpp
  operations/selection_tests.cpp
  main.cpp)

target_link_libraries(tests PRIVATE
  GTest::gtest
  GTest::gmock
  gal20_lib)

target_compile_features(tests PRIVATE cxx_std_20)
set_target_properties(tests PROPERTIES
	CXX_EXTENSIONS NO
	CXX_STANDARD_REQUIRED YES)

include(GoogleTest)
gtest_discover_tests(tests)
