
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
concept integer_fitness = arithmetic_fitness<Fitness> && std::integral<Fitness>;

template<typename Fitness>
concept real_fitness =
    arithmetic_fitness<Fitness> && std::floating_point<Fitness>;

template<typename Totalizator>
concept fitness_totalizator = requires(Totalizator t) {
  std::semiregular<Totalizator>;
  arithmetic_fitness<typename Totalizator::value_t>;

  {
    t.add(std::declval<typename Totalizator::value_t>())
    } -> std::convertible_to<Totalizator>;

  { t.sum() } -> std::convertible_to<typename Totalizator::value_t>;
};

template<arithmetic_fitness Value>
class integer_fitness_totalizator {
public:
  using value_t = Value;

public:
  inline integer_fitness_totalizator() = default;

  inline auto add(value_t const& value) const noexcept {
    return integer_fitness_totalizator{sum_ + value};
  }

  inline value_t const& sum() const noexcept {
    return sum_;
  }

private:
  inline integer_fitness_totalizator(value_t const& sum)
      : sum_{sum} {
  }

private:
  value_t sum_{};
};

template<arithmetic_fitness Value>
class real_fitness_totalizator {
public:
  using value_t = Value;

public:
  inline real_fitness_totalizator() = default;

  inline auto add(value_t const& value) const noexcept {
    value_t y = value - correction_;
    value_t t = sum_ + y;
    return real_fitness_totalizator{t, (t - sum_) - y};
  }

  inline value_t const& sum() const noexcept {
    return sum_;
  }

private:
  inline real_fitness_totalizator(value_t const& sum, value_t const& correction)
      : sum_{sum}
      , correction_{correction} {
  }

private:
  value_t sum_{};
  value_t correction_{};
};

template<fitness Fitness>
struct fitness_traits;

template<integer_fitness Fitness>
struct fitness_traits<Fitness> {
  using totalizator_t = integer_fitness_totalizator<Fitness>;
  using random_distribution_t = std::uniform_int_distribution<Fitness>;
};

template<real_fitness Fitness>
struct fitness_traits<Fitness> {
  using totalizator_t = real_fitness_totalizator<Fitness>;
  using random_distribution_t = std::uniform_real_distribution<Fitness>;
};

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

template<typename Scaling, typename Population>
concept scaling = std::is_invocable_v<
    Scaling,
    std::add_lvalue_reference_t<typename Population::individual_t>>;

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

struct empty_fitness {};
struct empty_tags {};

template<typename Fitness>
struct is_empty_fitness : std::is_same<Fitness, empty_fitness> {};

template<typename Fitness>
inline constexpr auto is_empty_fitness_v = is_empty_fitness<Fitness>::value;

} // namespace gal
