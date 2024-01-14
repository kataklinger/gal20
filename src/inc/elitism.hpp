
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace elite {

  template<typename Preserved>
  concept strict_elitist_set = std::same_as<Preserved, pareto_preserved_t> ||
                               std::same_as<Preserved, pareto_reduced_t> ||
                               std::same_as<Preserved, pareto_nondominated_t>;

  template<typename Preserved>
  concept relaxed_elitist_set =
      strict_elitist_set<Preserved> || std::same_as<Preserved, pareto_erased_t>;

  // only non-dominated (pesa, pesa-ii, paes)
  class strict {
  public:
    template<typename Population, strict_elitist_set Preserved>
      requires(!std::same_as<Preserved, pareto_erased_t>)
    inline void operator()(
        Population& population,
        population_pareto_t<Population, Preserved>& sets) const noexcept {
      if (sets.get_size_of(1) < population.current_size()) {
        sets.trim();

        population.remove_if([](auto const& individual) {
          return get_tag<frontier_level_t>(individual) != 1;
        });
      }
    }
  };

  // do not remove dominated (nsga, nsga-ii, spea, spea-ii, rdga)
  class relaxed {
  public:
    template<typename Population, relaxed_elitist_set Preserved>
    inline void operator()(
        Population& /*unused*/,
        population_pareto_t<Population, Preserved>& /*unused*/) const noexcept {
    }
  };

} // namespace elite
} // namespace gal
