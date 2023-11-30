cmake_minimum_required(VERSION 3.26)

add_library(warnings INTERFACE)

if(MSVC)
  target_compile_options(warnings INTERFACE /W4 /WX)
else()
  target_compile_options(warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(gal20_lib STATIC scrap.cpp)

target_link_libraries(gal20_lib PRIVATE warnings)

target_include_directories(
  gal20_lib PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

list(APPEND HEADER_LIST
  chromosome.hpp
  clustering.hpp
  configuration.hpp
  context.hpp
  coupling.hpp
  criteria.hpp
  crossover.hpp
  crowding.hpp
  elitism.hpp
  fitness.hpp
  individual.hpp
  multiobjective.hpp
  mutation.hpp
  observing.hpp
  operation.hpp
  pareto.hpp
  population.hpp
  projection.hpp
  pruning.hpp
  ranking.hpp
  replacement.hpp
  sampling.hpp
  scaling.hpp
  selection.hpp
  soo.hpp
  statistics.hpp
  utility.hpp)

list(TRANSFORM HEADER_LIST PREPEND $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
list(TRANSFORM HEADER_LIST PREPEND $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc/>)

target_sources(
  gal20_lib PUBLIC
  FILE_SET gal20_lib_headers
  TYPE HEADERS
  FILES ${HEADER_LIST}
  BASE_DIRS
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

target_compile_features(gal20_lib PUBLIC cxx_std_20)
set_target_properties(
    gal20_lib PROPERTIES
    CXX_EXTENSIONS NO
    CXX_STANDARD_REQUIRED YES)
