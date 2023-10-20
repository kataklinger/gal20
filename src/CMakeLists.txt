cmake_minimum_required(VERSION 3.26)

add_library(warnings INTERFACE)

#[==[
if(MSVC)
  target_compile_options(warnings INTERFACE /W4 /WX)
else()
  target_compile_options(warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()
]==]

add_library(gal20_lib STATIC scrap.cpp)

target_link_libraries(gal20_lib PRIVATE warnings)

target_include_directories(
  gal20_lib PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)

target_sources(
  gal20_lib PUBLIC
  FILE_SET gal20_lib_headers
  TYPE HEADERS
  FILES
    "inc/chromosome.hpp"
    "inc/configuration.hpp"
    "inc/context.hpp"
    "inc/coupling.hpp"
    "inc/criteria.hpp"
    "inc/crossover.hpp"
    "inc/elitism.hpp"
    "inc/fitness.hpp"
    "inc/individual.hpp"
    "inc/mutation.hpp"
    "inc/niching.hpp"
    "inc/observing.hpp"
    "inc/operation.hpp"
    "inc/pareto.hpp"
    "inc/population.hpp"
    "inc/projection.hpp"
    "inc/pruning.hpp"
    "inc/ranking.hpp"
    "inc/replacement.hpp"
    "inc/sampling.hpp"
    "inc/scaling.hpp"
    "inc/selection.hpp"
    "inc/soo.hpp"
    "inc/statistics.hpp"
    "inc/utility.hpp"
  BASE_DIRS inc)

target_compile_features(gal20_lib PUBLIC cxx_std_20)
set_target_properties(
    gal20_lib PROPERTIES
    CXX_EXTENSIONS NO
    CXX_STANDARD_REQUIRED YES)
