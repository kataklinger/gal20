
#pragma once

namespace gal {

template<typename Population, typename Statistics>
class population_context {
public:
  using population_t = Population;
  using statistics_t = Statistics;

public:
  constexpr inline population_context(population_t& population,
                                      statistics_t& statistics) noexcept
      : population_{&population}
      , statistics_{&statistics} {
  }

  inline population_t& population() noexcept {
    return *population_;
  }

  inline population_t const& population() const noexcept {
    return *population_;
  }

  inline statistics_t& statistics() noexcept {
    return *statistics_;
  }

  inline statistics_t const& statistics() const noexcept {
    return *statistics_;
  }

private:
  population_t* population_;
  statistics_t* statistics_;
};

template<typename Population,
         typename Statistics,
         typename Crossover,
         typename Mutation,
         typename Evaluator>
class reproduction_context : population_context<Population, Statistics> {
public:
  using population_t = Population;
  using statistics_t = Statistics;
  using crossover_t = Crossover;
  using mutation_t = Mutation;
  using evaluator_t = Evaluator;

private:
  using base_t = population_context<Population, Statistics>;

public:
  constexpr inline reproduction_context(population_t& population,
                                        statistics_t& statistics,
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

private:
  using base_t = reproduction_context<Population,
                                      Statistics,
                                      Crossover,
                                      Mutation,
                                      Evaluator>;

public:
  constexpr inline reproduction_context_with_scaling(
      population_t& population,
      statistics_t& statistics,
      crossover_t const& crossover,
      mutation_t const& mutation,
      evaluator_t const& evaluator,
      scaling_t const& scaling) noexcept
      : base_t{population, statistics, crossover, mutation, evaluator}
      , scaling_{scaling} {
  }

  inline scaling_t const& scaling() const noexcept {
    return scaling_;
  }

private:
  scaling_t scaling_;
};

} // namespace gal
