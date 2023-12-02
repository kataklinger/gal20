
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
    inline void operator()(Population& population) const {
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

  namespace details {
    template<typename Population>
    inline void prune_population(Population& population) noexcept {
      population.remove_if([](auto const& individual) {
        return get_tag<prune_state_t>(individual);
      });
    }
  } // namespace details

  // remove random from cluster (pesa, pesa-ii, paes)
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
    void operator()(Population& population, cluster_set& clusters) const {
      std::vector<std::size_t> selected(clusters.size());

      std::size_t i = 0;
      for (auto&& cluster_size : clusters) {
        std::uniform_int_distribution<std::size_t> dist{0, cluster_size - 1};
        selected[i++] = dist(*generator_);

        cluster_size = 1;
      }

      for (auto&& individual : population.individuals()) {
        auto& to_prune = get_tag<prune_state_t>(individual);
        auto label = get_tag<cluster_label>(individual);

        if (label.is_proper()) {
          auto left = --selected[label.index()];
          to_prune = left != 0;
        }
        else {
          to_prune = !label.is_unique();
        }
      }

      details::prune_population(population);
    }

  private:
    generator_t* generator_;
  };

  // remove non-centroids from cluster (spea)
  class cluster_edge {
  public:
    template<prunable_population Population>
      requires(clustered_population<Population> &&
               spatial_population<Population>)
    void operator()(Population& population, cluster_set& clusters) const {
      using individual_t = typename Population::individual_t;

      std::vector<std::tuple<std::size_t, std::size_t>> buffer_index;

      std::size_t buffer_size = 0;
      for (auto&& cluster_size : clusters) {
        buffer_index.emplace_back(buffer_size, 0);
        buffer_size += cluster_size;
        cluster_size = 1;
      }

      std::vector<std::tuple<individual_t*, double>> buffer(buffer_size);

      for (auto&& individual : population.individuals()) {
        auto label = get_tag<cluster_label>(individual);

        if (label.is_proper()) {
          auto& [first, current] = buffer_index[label.index()];

          double total_distance = 0.;
          for (; first <= current; ++first) {
            auto& [other_individual, other_distance] = buffer[first];

            auto distance =
                euclidean_distance(individual.evaluation().raw(),
                                   other_individual->evaluation().raw());

            other_distance += distance;
            total_distance += distance;
          }

          buffer[++current] = std::tuple{&individual, total_distance};

          get_tag<prune_state_t>(individual) = true;

          if (current - first == clusters[label.index()]) {
            auto& [center, dummy] = *std::ranges::min_element(
                buffer.begin() + first,
                buffer.begin() + current,
                std::less{},
                [](auto& element) { std::get<1>(element); });

            get_tag<prune_state_t>(center) = false;
          }
        }
        else {
          get_tag<prune_state_t>(individual) = !label.is_unique();
        }
      }

      details::prune_population(population);
    }
  };

} // namespace prune
} // namespace gal
