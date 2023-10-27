
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
concept mutation = std::
    is_invocable_r_v<void, Operation, std::add_lvalue_reference_t<Chromosome>>;

template<typename Operation, typename Chromosome>
concept evaluator =
    std::is_invocable_v<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>> &&
    fitness<std::invoke_result_t<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>>;

template<typename Operation, typename Chromosome>
concept proximation =
    std::is_invocable_v<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>> &&
    std::convertible_to<
        std::invoke_result_t<
            Operation,
            std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
            std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>,
        double>;

template<chromosome Chromosome, evaluator<Chromosome> Evaluator>
using get_evaluator_result_t = std::invoke_result_t<
    Evaluator,
    std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>;

namespace details {
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
  using is_stable_t = typename details::is_stable_scaling_helper<Scaling>::type;
};

template<typename Scaling, typename Population>
struct can_scale_local
    : std::conjunction<
          std::negation<std::is_invocable_r<void, Scaling>>,
          std::is_invocable_r<
              void,
              Scaling,
              std::add_lvalue_reference_t<typename Population::individual_t>>> {
};

template<typename Scaling, typename Population>
inline constexpr auto can_scale_local_v =
    can_scale_local<Scaling, Population>::value;

template<typename Scaling, typename Population>
struct can_scale_global
    : std::is_invocable_r<
          void,
          Scaling,
          std::size_t,
          std::add_lvalue_reference_t<typename Population::individual_t>> {};

template<typename Scaling, typename Population>
inline constexpr auto can_scale_global_v =
    can_scale_global<Scaling, Population>::value;

template<typename Scaling, typename Population>
concept local_scaling = can_scale_local_v<Scaling, Population> &&
                        !can_scale_global_v<Scaling, Population>;

template<typename Scaling, typename Population>
concept global_scaling = can_scale_global_v<Scaling, Population> &&
                         !can_scale_local_v<Scaling, Population>;

template<typename Scaling, typename Population>
concept scaling =
    local_scaling<Scaling, Population> || global_scaling<Scaling, Population>;

template<typename Operation, typename Population>
concept selection =
    std::is_invocable_v<Operation, std::add_lvalue_reference_t<Population>> &&
    selection_range<
        std::invoke_result_t<Operation,
                             std::add_lvalue_reference_t<Population>>,
        typename Population::iterator_t>;

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

template<typename Operation, typename Population, typename History>
concept criterion = std::is_invocable_r_v<
    bool,
    Operation,
    std::add_lvalue_reference_t<std::add_const_t<Population>>,
    std::add_lvalue_reference_t<std::add_const_t<History>>>;

template<typename Factory, typename Context>
concept scaling_factory =
    std::is_invocable_v<std::add_const_t<Factory>,
                        std::add_lvalue_reference_t<Context>> &&
    scaling<std::invoke_result_t<std::add_const_t<Factory>,
                                 std::add_lvalue_reference_t<Context>>,
            typename Context::population_t>;

template<typename Factory, typename Context, typename Parents>
concept coupling_factory =
    std::is_invocable_v<std::add_const_t<Factory>,
                        std::add_lvalue_reference_t<Context>> &&
    coupling<std::invoke_result_t<std::add_const_t<Factory>,
                                  std::add_lvalue_reference_t<Context>>,
             typename Context::population_t,
             Parents>;

template<typename Factory, typename Context>
using factory_result_t =
    std::invoke_result_t<std::add_const_t<Factory>,
                         std::add_lvalue_reference_t<Context>>;

template<auto Probability>
concept probability = Probability >= 0.f && Probability <= 1.f &&
                      std::floating_point<decltype(Probability)>;

template<typename Generator, auto Probability>
  requires(probability<Probability>)
struct probabilistic_operation {
public:
  using generator_t = Generator;
  using distribution_t = std::uniform_real_distribution<decltype(Probability)>;

public:
  inline explicit probabilistic_operation(generator_t& generator) noexcept
      : generator_{&generator} {
  }

  inline bool operator()() const {
    if constexpr (Probability == 0.f) {
      return false;
    }
    else if constexpr (Probability == 1.f) {
      return true;
    }

    return distribution_t{0.f, 1.f}(*generator_) < Probability;
  }

private:
  generator_t* generator_;
};

struct scaling_time_t {};

inline constexpr scaling_time_t scaling_time_tag{};

struct selection_time_t {};
struct selection_count_t {};

inline constexpr selection_time_t selection_time_tag{};
inline constexpr selection_count_t selection_count_tag{};

struct crossover_count_t {};
struct mutation_tried_count_t {};
struct mutation_accepted_count_t {};

inline constexpr crossover_count_t crossover_count_tag{};
inline constexpr mutation_tried_count_t mutation_tried_count_tag{};
inline constexpr mutation_accepted_count_t mutation_accepted_count_tag{};

struct coupling_count_t {};
struct coupling_time_t {};

inline constexpr coupling_count_t coupling_count_tag{};
inline constexpr coupling_time_t coupling_time_tag{};

struct replacement_time_t {};
struct replacement_count_t {};

inline constexpr replacement_time_t replacement_time_tag{};
inline constexpr replacement_count_t replacement_count_tag{};

} // namespace gal
