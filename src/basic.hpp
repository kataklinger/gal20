
#pragma once

#include <concepts>
#include <random>
#include <ranges>
#include <type_traits>

namespace gal {

template<typename Type>
concept fitness =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

template<typename Type>
concept ordered_fitness = fitness<Type> && std::totally_ordered<Type>;

template<typename Type>
concept arithmetic_fitness = fitness<Type> && requires(Type a) {
  { a + a } -> std::same_as<Type>;
  { a - a } -> std::same_as<Type>;
  { a / std::size_t(1) } -> std::same_as<Type>;
};

template<typename Fitness>
struct random_fitness_distribution;

template<std::integral Fitness>
struct random_fitness_distribution<Fitness> {
  using type = std::uniform_int_distribution<Fitness>;
};

template<std::floating_point Fitness>
struct random_fitness_distribution<Fitness> {
  using type = std::uniform_real_distribution<Fitness>;
};

template<typename Fitness>
using random_fitness_distribution_t =
    typename random_fitness_distribution<Fitness>::type;

template<typename Type>
concept chromosome =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

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

template<typename Scaling, typename Chromosome, typename Raw, typename Scaled>
concept scaling = std::is_invocable_v<
    Scaling,
    std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
    std::add_lvalue_reference_t<std::add_const_t<Raw>>,
    std::add_lvalue_reference_t<Scaled>>;

template<typename Scaling>
struct scaling_traits {
  using is_global_t = std::is_invocable<Scaling>;
  using is_stable_t = std::false_type;
};

template<typename Range, typename It>
concept selection_range = std::ranges::range<Range> && requires(Range r) {
  requires std::same_as<std::remove_cvref_t<decltype(*std::ranges::begin(r))>,
                        It>;
};

template<typename Range, typename It, typename Chromosome>
concept replacement_range = std::ranges::range<Range> && requires(Range r) {
  requires std::same_as<
      std::remove_cvref_t<decltype(get_parent(*std::ranges::begin(r)))>,
      It>;
  requires std::same_as<
      std::remove_cvref_t<decltype(get_child(*std::ranges::begin(r)))>,
      Chromosome>;
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
                typename Context::population_t::chromosome_t,
                typename Context::population_t::raw_fitness_t,
                typename Context::population_t::scaled_fitness_t>;

template<typename Factory, typename Context, typename Parents>
concept coupling_factory =
    std::is_invocable_v<Factory, Context> &&
        coupling<std::invoke_result_t<Factory, Context>,
                typename Context::population_t,
                Parents>;

// clang-format on

template<typename Factory, typename Context>
using factory_result_t = std::invoke_result_t<Factory, Context>;

struct empty_fitness {};
struct empty_tags {};

template<typename Fitness>
struct is_empty_fitness : std::is_same<Fitness, empty_fitness> {};

template<typename Fitness>
inline constexpr auto is_empty_fitness_v = is_empty_fitness<Fitness>::value;

} // namespace gal
