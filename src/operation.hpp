
#pragma once

#include "population.hpp"

#include <random>

namespace gal {

template<typename Operation>
concept initializator = std::is_invocable_v<Operation> &&
    chromosome<std::invoke_result_t<Operation>>;

template<typename Operation, typename Chromosome>
concept crossover = std::is_invocable_r_v<
    std::pair<Chromosome, Chromosome>,
    Operation,
    std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
    std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>;

template<typename Operation, typename Chromosome>
concept mutation =
    std::is_invocable_v<Operation, std::add_lvalue_reference_t<Chromosome>>;

template<typename Operation, typename Chromosome>
concept evaluator = std::is_invocable_v<
    Operation,
    std::add_lvalue_reference_t<std::add_const_t<Chromosome>>> &&
    fitness<std::invoke_result_t<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>>;

template<typename Scaling, typename Population>
concept scaling = std::is_invocable_v<
    Scaling,
    std::add_lvalue_reference_t<typename Population::individual_t>>;

template<typename Scaling>
struct scaling_traits {
  using is_global_t = std::is_invocable<Scaling>;
  using is_stable_t = std::false_type;
};

// clang-format off

template<typename Operation, typename Population>
concept selection =
    std::is_invocable_v<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Population>>> &&
    selection_range<
        std::invoke_result_t<Operation,
                             std::add_lvalue_reference_t<Population>>,
        typename Population::const_iterator_t>;

struct coupling_metadata {
  bool crossover_performed;
  bool mutation_tried;
  bool mutation_accepted;
};

template<typename Operation, typename Population, typename Parents>
concept coupling = std::is_invocable_v<Operation, Parents> &&
                   replacement_range<std::invoke_result_t<Operation, Parents>,
                                     typename Population::const_iterator_t,
                                     typename Population::individual_t>;

template<typename Operation, typename Population, typename Offsprings>
concept replacement =
    std::is_invocable_v<Operation,
                        std::add_lvalue_reference_t<Population>,
                        Offsprings> &&
    selection_range<
        std::invoke_result_t<Operation,
                             std::add_lvalue_reference_t<Population>,
                             Offsprings>,
        typename Population::individual_t>;

// clang-format on

template<typename Operation, typename Population, typename Statistics>
concept criterion =
    std::is_invocable_r_v<bool, Operation, Population, Statistics>;

// clang-format off

template<typename Factory, typename Context>
concept scaling_factory =
    std::is_invocable_v<Factory, Context> &&
        scaling<std::invoke_result_t<Factory, Context>,
                typename Context::population_t>;

template<typename Factory, typename Context, typename Parents>
concept coupling_factory =
    std::is_invocable_v<Factory, Context> &&
        coupling<std::invoke_result_t<Factory, Context>,
                typename Context::population_t,
                Parents>;

// clang-format on

template<typename Factory, typename Context>
using factory_result_t = std::invoke_result_t<Factory, Context>;

} // namespace gal
