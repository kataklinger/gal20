cmake_minimum_required(VERSION 3.26)

find_package(GTest CONFIG REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(tests
  operations/multiobjective/pareto_tests.cpp
  operations/multiobjective/ranking_tests.cpp
  operations/multiobjective/elitism_tests.cpp
  operations/multiobjective/clustering_tests.cpp
  operations/multiobjective/crowding_tests.cpp
  operations/multiobjective/pruning_tests.cpp
  operations/multiobjective/projection_tests.cpp
  operations/population/coupling_tests.cpp
  operations/population/criteria_tests.cpp
  operations/population/replacement_tests.cpp
  operations/population/scaling_tests.cpp
  operations/population/selection_tests.cpp
  operations/chromosome/mutation_tests.cpp
  operations/chromosome/crossover_tests.cpp
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
