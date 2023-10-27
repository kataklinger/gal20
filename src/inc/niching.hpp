
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

#include <cmath>

namespace gal {
namespace niche {

  // fitness sharing (nsga)
  template<chromosome Chromosome,
           proximation<Chromosome> Proximation,
           double Cutoff,
           double Alpha = 2.>
  class share {
  public:
    using proximation_t = Proximation;

    inline constexpr double cutoff = Cutoff;
    inline constexpr double alpha = Alpha;

  public:
    inline explicit share(proximation_t proximity)
        : proximity_{proximity} {
    }

  public:
    template<density_population Population>
    void
        operator()(Population& population,
                   pareto_sets<typename Population::individual_t>& sets) const {
      clean_tags<density_value_t>(population);

      for (auto&& set : sets) {
        for (auto&& left : set) {
          for (auto&& right :
               std::ranges::subrange{std::ranges::next(std::ranges::begin(set)),
                                     std::ranges::end(set)}) {
            auto proximity = static_cast<double>(
                proximity_(left->chromosome(), right->chromosome()));

            auto niching = proximity < cutoff
                               ? 1. - std::pow(proximity / cutoff, alpha)
                               : 0.;

            get_tag<density_value_t>(*left) += niching;
            get_tag<density_value_t>(*right) += niching;
          }
        }
      }
    }

  private:
    proximation_t proximity_;
  };

  // crowding distance (nsga-ii)
  class distance {
  public:
    template<typename Population>
      requires(density_population<Population> &&
               crowding_population<Population>)
    void operator()(Population& population,
                    pareto_sets<typename Population::individual_t>& sets) {
      clean_tags<density_value_t>(population);

      auto objectives =
          std::ranges::size(population.individuals().evaluation().raw());

      for (auto&& set : sets) {
        for (std::size_t objective = 0; objective < objectives; ++objective) {
          std::ranges::sort(set, [objective](auto const& lhs, auto const& rhs) {
            return lhs->evaluation().raw()[objective] <
                   rhs->evaluation().raw()[objective];
          });

          auto first = std::ranges::begin(set);
          auto last = --std::ranges::end(set);

          get_tag<density_tag>(**first) = std::numeric_limits<float>::max();
          get_tag<density_tag>(**last) = std::numeric_limits<float>::max();

          while (++first != last) {
            auto distance = static_cast<double>(
                (*(first + 1))->evaluation().raw()[objective] -
                (*(first - 1))->evaluation().raw()[objective]);

            get_tag<density_tag>(**first) += distance;
          }
        }
      }
    }
  };

  class cluster {}; // average linkage (spea)

  // kth nearest neighbor (spea-ii)
  class neighbor {};

  // cell-sharing (pesa, pesa-ii, paes)
  class hypergrid {};

  // adaptive cell-sharing (rdga)
  class adaptive_hypergrid {};

} // namespace niche
} // namespace gal
