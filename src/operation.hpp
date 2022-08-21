
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
    rank_t,
    std::add_lvalue_reference_t<typename Population::individual_t>>;

namespace details {
  template<typename Scaling, typename = void>
  struct is_global_scaling_helper {
    using type = std::is_invocable<Scaling>;
  };

  template<typename Scaling>
  struct is_global_scaling_helper<Scaling,
                                  std::void_t<typename Scaling::is_global_t>> {
    using type = typename Scaling::is_global_t;
  };

  template<typename Scaling, typename = void>
  struct is_stable_scaling_helper {
    using type = std::false_type;
  };

  template<typename Scaling>
  struct is_stable_scaling_helper<Scaling,
                                  std::void_t<typename Scaling::is_stable_t>> {
    using type = typename Scaling::is_stable_t;
  };
} // namespace details

template<typename Scaling>
struct scaling_traits {
  using is_global_t = typename details::is_global_scaling_helper<Scaling>::type;
  using is_stable_t = typename details::is_stable_scaling_helper<Scaling>::type;
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
        typename Population::iterator_t>;

struct coupling_metadata {
  std::size_t crossover_performed;
  std::size_t mutation_tried;
  std::size_t mutation_accepted;
};

template<typename Operation, typename Population, typename Parents>
concept coupling = std::is_invocable_v<Operation, Parents> &&
                   replacement_range<std::invoke_result_t<Operation, Parents>,
                                     typename Population::iterator_t,
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
