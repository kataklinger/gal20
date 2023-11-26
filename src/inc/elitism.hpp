
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace elite {

  template<typename Pareto>
  concept strict_elitist_set = std::same_as<Pareto, pareto_preserved_t> ||
                               std::same_as<Pareto, pareto_reduced_t> ||
                               std::same_as<Pareto, pareto_nondominated_t>;

  template<typename Pareto>
  concept relaxed_elitist_set =
      strict_elitist_set<Pareto> || std::same_as<Pareto, pareto_erased_t>;

  // only non-dominated (pesa, pesa-ii, paes)
  class strict {
  public:
    template<typename Population, strict_elitist_set Pareto>
      requires(!std::same_as<Pareto, pareto_erased_t>)
    inline void operator()(Population& population,
                           population_pareto_t<Population>& sets,
                           Pareto /*unused*/) const noexcept {
      if (!sets.empty() && sets.get_count_of(1) < population.current_size()) {
        sets.trim();

        population.remove_if([](auto const& individual) {
          return get_tag<frontier_level_t>(individual) != 1;
        });
      }
    }
  };

  // do not remove dominated (nsga, nsga-ii, spea, spea-ii)
  class relaxed {
  public:
    template<typename Population, relaxed_elitist_set Pareto>
    inline void operator()(Population& /*unused*/,
                           population_pareto_t<Population>& /*unused*/,
                           Pareto /*unused*/) const noexcept {
    }
  };

} // namespace elite
} // namespace gal
