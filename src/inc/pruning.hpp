
#pragma once

#include "multiobjective.hpp"

#include <random>

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
  template<typename Generator>
  class cluster_random {
  public:
    using generator_t = Generator;

  public:
    inline explicit cluster_random(generator_t& generator) noexcept
        : generator_{generator} {
    }

    template<clustered_population Population>
      requires(prunable_population<Population>)
    void operator()(Population& population, cluster_set const& clusters) const {
      std::vector<std::size_t> selected(clusters.size());

      std::size_t i = 0;
      for (auto&& cluster_size : clusters) {
        std::uniform_int_distribution<std::size_t> dist{0, cluster_size - 1};
        selected[i++] = dist(*generator_);
      }

      for (auto&& individual : population.individuals()) {
        auto& state = get_tag<prune_state_t>(individual);
        auto label = get_tag<cluster_label>(individual);

        if (label.is_proper()) {
          state = (--selected[label.index()]) != 0
        }
        else {
          state = !label.is_unique();
        }
      }

      population.remove_if([](auto const& individual) {
        return get_tag<prune_state_t>(individual);
      });
    }

  private:
    generator_t* generator_;
  };

  // remove non-centroids (spea)
  class cluster_edge {
  public:
    template<clustered_population Population>
      requires(prunable_population<Population>)
    void operator()(Population& population, cluster_set const& clusters) const {
    }
  };

} // namespace prune
} // namespace gal
