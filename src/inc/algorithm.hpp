#pragma once

#include "configuration.hpp"

namespace gal {

struct generation_event_t {};
inline constexpr generation_event_t generation_event{};

template<>
struct observer_definition<generation_event_t> {
  template<typename Observer, typename Definitions>
  inline static constexpr auto satisfies = std::is_invocable_v<
      Observer,
      std::add_lvalue_reference_t<
          std::add_const_t<typename Definitions::population_t>>,
      std::add_lvalue_reference_t<std::add_const_t<
          stats::history<typename Definitions::statistics_t>>>>;
};

template<typename Config>
concept basic_algo_config = requires(Config c) {
  requires chromosome<typename Config::chromosome_t>;
  requires fitness<typename Config::raw_fitness_t>;
  requires fitness<typename Config::scaled_fitness_t>;

  requires std::same_as<typename Config::population_t,
                        population<typename Config::chromosome_t,
                                   typename Config::raw_fitness_t,
                                   typename Config::raw_comparator_t,
                                   typename Config::scaled_fitness_t,
                                   typename Config::scaled_comparator_t,
                                   typename Config::tags_t>>;

  requires initializator<typename Config::initializator_t>;
  requires crossover<typename Config::crossover_t,
                     typename Config::chromosome_t>;
  requires mutation<typename Config::mutation_t, typename Config::chromosome_t>;
  requires evaluator<typename Config::evaluator_t,
                     typename Config::chromosome_t>;

  requires comparator<typename Config::raw_comparator_t,
                      typename Config::raw_fitness_t>;

  requires comparator<typename Config::scaled_comparator_t,
                      typename Config::raw_fitness_t>;

  requires selection_range<typename Config::selection_result_t,
                           typename Config::population_t::iterator_t>;
  requires replacement_range<typename Config::copuling_result_t,
                             typename Config::population_t::iterator_t,
                             typename Config::population_t::individual_t>;

  requires selection<typename Config::selection_t,
                     typename Config::population_t>;
  requires coupling<typename Config::coupling_t,
                    typename Config::population_t,
                    typename Config::selection_result_t>;
  requires replacement<typename Config::replacement_t,
                       typename Config::population_t,
                       typename Config::copuling_result_t>;

  requires criterion<typename Config::criterion_t,
                     typename Config::population_t,
                     typename Config::history_t>;

  {
    c.initializator()
  } -> std::convertible_to<typename Config::initializator_t>;

  { c.evaluator() } -> std::convertible_to<typename Config::evaluator_t>;

  {
    c.raw_comparator()
  } -> std::convertible_to<typename Config::raw_comparator_t>;

  {
    c.scaled_comparator()
  } -> std::convertible_to<typename Config::scaled_comparator_t>;

  { c.selection() } -> std::convertible_to<typename Config::selection_t>;

  {
    c.coupling(std::declval<typename Config::reproduction_context_t&>())
  } -> std::convertible_to<typename Config::coupling_t>;

  { c.criterion() } -> std::convertible_to<typename Config::criterion_t>;

  { c.observers() };
  { c.population_size() } -> std::convertible_to<std::size_t>;
  { c.statistics_depth() } -> std::convertible_to<std::size_t>;
};

} // namespace gal
