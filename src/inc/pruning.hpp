
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace prune {

  class none {
  public:
    template<multiobjective_population Population>
    inline void operator()(Population& /*unused*/,
                           cluster_set const& /*unused*/) const {
    }
  };

  // top by assigned <rank, density> (nsga-ii, spea-ii)
  template<typename RankTag>
  class global_worst {
  public:
    template<crowded_population Population>
      requires(ranked_population<Population, RankTag>)
    inline void operator()(Population& population,
                           cluster_set const& /*unused*/) const {
      population.sort([](auto const& lhs, auto const& rhs) {
        auto flhs = get_tag<RankTag>(lhs);
        auto frhs = get_tag<RankTag>(rhs);

        return flhs < frhs ||
               (flhs == frhs &&
                get_tag<crowd_density_t>(lhs) < get_tag<crowd_density_t>(rhs));
      });

      population.trim();
    }
  };

  // remove random from group (pesa, pesa-ii, paes)
  class cluster_random {
    template<clustered_population Population>
    void operator()(Population& population, cluster_set const& clusters) const {
    }
  };

  // remove non-centroids (spea)
  class cluster_edge {
    template<clustered_population Population>
    void operator()(Population& population, cluster_set const& clusters) const {
    }
  };

} // namespace prune
} // namespace gal
