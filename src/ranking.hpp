
#pragma once

#include "pareto.hpp"
#include "population.hpp"

#include <concepts>

namespace gal {
namespace rank {

  template<typename Value>
    requires std::integral<Value> || std::floating_point<Value>
  class rank {
  public:
    using value_t = Value;

  public:
    inline rank(value_t value) noexcept
        : value_{value} {
    }

    inline operator value_t() const noexcept {
      return value_;
    }

    inline value_t get() const noexcept {
      return value_;
    }

  private:
    value_t value_;
  };

  template<typename Population, typename Rank>
  concept ranked_population =
      multiobjective_population<Population> && tagged_with<Population, Rank>;

  namespace details {

    template<typename Rank, typename Individual>
    inline auto& ranking(Individual& individual) noexcept {
      return std::get<Rank>(individual.tags());
    }

    template<typename Rank, typename Individual>
    inline auto const& ranking(Individual const& individual) noexcept {
      return std::get<Rank>(individual.tags());
    }

    template<typename Rank, ranked_population<Rank> Population>
    inline void clean(Population& population) {
      for (auto& individual : population.individuals()) {
        ranking<Rank>(individual) = {};
      }
    }

  } // namespace details

  // non-dominated vs dominated (pesa, pesa-ii, paes)
  class binary {};

  using integral_rank_t = rank<std::size_t>;

  // pareto front level (nsga & nsga-ii)
  class level {
  public:
    template<ranked_population<integral_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<integral_rank_t>(population);

      for (auto& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto& individual : frontier.members()) {
          details::ranking<integral_rank_t>(individual) = frontier.level();
        }
      }
    }
  };

  // number of dominated, first front (spea)
  class strength {};

  // number of dominated, all fronts (spea-ii)
  class acc_strength {};

  // accumulated pareto level (rdga)
  class acc_level {
    template<ranked_population<integral_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<integral_rank_t>(population);

      for (auto& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto& individual : frontier.members()) {
          details::ranking<integral_rank_t>(individual) += 1;

          for (auto& dominated : individual.dominated()) {
            details::ranking<integral_rank_t>(dominated) +=
                details::ranking<integral_rank_t>(individual);
          }
        }
      }
    }
  };

} // namespace rank
} // namespace gal
