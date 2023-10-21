
#pragma once

#include "statistics.hpp"

namespace gal {

template<typename Population, typename Statistics>
class population_context {
public:
  using population_t = Population;
  using statistics_t = Statistics;

  using history_t = stats::history<statistics_t>;

public:
  inline constexpr population_context(population_t& population,
                                      history_t& statistics) noexcept
      : population_{&population}
      , statistics_{&statistics} {
  }

  inline auto& population() noexcept {
    return *population_;
  }

  inline auto const& population() const noexcept {
    return *population_;
  }

  inline auto& history() noexcept {
    return *statistics_;
  }

  inline auto const& history() const noexcept {
    return *statistics_;
  }

  inline auto& comparator() const noexcept {
    return population_->raw_comparator();
  }

private:
  population_t* population_;
  history_t* statistics_;
};

template<typename Population,
         typename Statistics,
         typename Crossover,
         typename Mutation,
         typename Evaluator>
class reproduction_context : public population_context<Population, Statistics> {
public:
  using population_t = Population;
  using statistics_t = Statistics;
  using crossover_t = Crossover;
  using mutation_t = Mutation;
  using evaluator_t = Evaluator;

  using history_t = stats::history<statistics_t>;

private:
  using base_t = population_context<Population, Statistics>;

public:
  inline constexpr reproduction_context(population_t& population,
                                        history_t& statistics,
                                        crossover_t const& crossover,
                                        mutation_t const& mutation,
                                        evaluator_t const& evaluator)
      : base_t{population, statistics}
      , crossover_{crossover}
      , mutation_{mutation}
      , evaluator_{evaluator} {
  }

  inline crossover_t const& crossover() const noexcept {
    return crossover_;
  }

  inline mutation_t const& mutation() const noexcept {
    return mutation_;
  }

  inline evaluator_t const& evaluator() const noexcept {
    return evaluator_;
  }

private:
  crossover_t crossover_;
  mutation_t mutation_;
  evaluator_t evaluator_;
};

template<typename Population,
         typename Statistics,
         typename Crossover,
         typename Mutation,
         typename Evaluator,
         typename Scaling>
class reproduction_context_with_scaling
    : public reproduction_context<Population,
                                  Statistics,
                                  Crossover,
                                  Mutation,
                                  Evaluator> {
public:
  using population_t = Population;
  using statistics_t = Statistics;
  using crossover_t = Crossover;
  using mutation_t = Mutation;
  using evaluator_t = Evaluator;
  using scaling_t = Scaling;

  using history_t = stats::history<statistics_t>;

private:
  using base_t = reproduction_context<Population,
                                      Statistics,
                                      Crossover,
                                      Mutation,
                                      Evaluator>;

public:
  inline constexpr reproduction_context_with_scaling(
      population_t& population,
      history_t& statistics,
      crossover_t const& crossover_op,
      mutation_t const& mutation_op,
      evaluator_t const& evaluator_op,
      scaling_t const& scaling_op) noexcept
      : base_t{population, statistics, crossover_op, mutation_op, evaluator_op}
      , scaling_{scaling_op} {
  }

  inline scaling_t const& scaling() const noexcept {
    return scaling_;
  }

private:
  scaling_t scaling_;
};

} // namespace gal
