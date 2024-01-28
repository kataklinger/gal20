
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

namespace gal {
namespace crowd {

  // no crowding (spea, pesa-ii)
  class none {
  public:
    template<multiobjective_population Population, typename Preserved>
    void operator()(Population& /*unused*/,
                    population_pareto_t<Population, Preserved>& /*unused*/,
                    cluster_set const& /*unused*/) const {
    }
  };

  // fitness sharing (nsga)
  template<typename Proximity>
  class sharing {
  public:
    using proximity_t = Proximity;

  public:
    inline sharing(
        double cutoff,
        double alpha,
        proximity_t const&
            prox) noexcept(std::is_nothrow_copy_constructible_v<proximity_t>)
        : proximity_{prox}
        , cutoff_{cutoff}
        , alpha_{alpha} {
    }

    template<typename... Args>
      requires(std::is_constructible_v<proximity_t, Args...>)
    inline sharing(double cutoff, double alpha, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<proximity_t, Args...>)
        : proximity_{std::forward<Args>(args)...}
        , cutoff_{cutoff}
        , alpha_{alpha} {
    }

  public:
    template<crowded_population Population, typename Preserved>
      requires(proximity<proximity_t, typename Population::chromosome_t>)
    void operator()(Population& population,
                    population_pareto_t<Population, Preserved>& sets,
                    cluster_set const& /*unused*/) const {
      clean_tags<crowd_density_t>(population);

      for (auto&& set : sets) {
        double total = 0.;

        for (auto in = std::ranges::begin(set); auto&& left : set) {
          std::ranges::advance(in, 1);
          for (auto&& right :
               std::ranges::subrange{in, std::ranges::end(set)}) {
            auto dist = static_cast<double>(
                proximity_(left->chromosome(), right->chromosome()));

            auto niching =
                dist < cutoff_ ? 1. - std::pow(dist / cutoff_, alpha_) : 0.;

            get_tag<crowd_density_t>(*left) += niching;
            get_tag<crowd_density_t>(*right) += niching;
            total += niching;
          }
        }

        for (auto&& individual : set) {
          auto& tag = get_tag<crowd_density_t>(*individual);
          tag = static_cast<double>(tag) / total;
        }
      }
    }

  private:
    proximity_t proximity_;
    double cutoff_;
    double alpha_;
  };

  // crowding distance (nsga-ii)
  class distance {
  public:
    template<crowded_population Population, typename Preserved>
      requires(spatial_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population, Preserved>& sets,
                    cluster_set const& /*unused*/) const {
      assert(population.current_size() > 0);

      clean_tags<crowd_density_t>(population);

      auto objectives =
          std::ranges::size(population.individuals()[0].evaluation().raw());

      constexpr auto objective = [](auto ind, std::size_t obj) {
        return ind->evaluation().raw()[obj];
      };

      for (auto&& set : sets) {
        for (std::size_t i = 0; i < objectives; ++i) {
          std::ranges::sort(set, [i](auto const& lhs, auto const& rhs) {
            return objective(lhs, i) < objective(rhs, i);
          });

          auto first = std::ranges::begin(set);
          auto last = --std::ranges::end(set);

          get_tag<crowd_density_t>(**first) = get_tag<crowd_density_t>(**last) =
              std::numeric_limits<double>::infinity();

          while (++first != last) {
            get_tag<crowd_density_t>(**first) += static_cast<double>(
                objective(*(first + 1), i) - objective(*(first - 1), i));
          }
        }

        auto min_distance = std::numeric_limits<double>::max();
        for (auto&& individual : set) {
          double distance{get_tag<crowd_density_t>(*individual)};
          min_distance = std::min(min_distance, distance);
        }

        for (auto&& individual : set) {
          auto& distance = get_tag<crowd_density_t>(*individual);
          distance = min_distance / (distance.get() + 1.);
        }
      }
    }
  };

  // kth nearest neighbor (spea-ii)
  class neighbor {
  public:
    template<crowded_population Population, typename Preserved>
      requires(spatial_population<Population>)
    inline void
        operator()(Population& population,
                   population_pareto_t<Population, Preserved>& /*unused*/,
                   cluster_set const& /*unused*/) const {
      auto distances = compute_distances(population.individuals());
      assign_density(distances, population.individuals());
    }

  private:
    template<typename Individuals>
    std::vector<double> compute_distances(Individuals& individuals) const {
      auto count = individuals.size();
      std::vector<double> distances(count * count);

      std::size_t i = 0, h = 0, v = 0;
      for (auto first = individuals.begin(), last = individuals.end();
           first != last;
           ++first) {

        h = v = i * count + i;
        distances[h] = distances[v] = 0;

        for (auto other = first + 1; other != last; ++other) {
          h += 1;
          v += count;

          distances[h] = distances[v] = euclidean_distance(
              first->evaluation().raw(), other->evaluation().raw());
        }

        ++i;
      }

      return distances;
    }

    template<typename Individuals>
    void assign_density(std::vector<double>& distances,
                        Individuals& individuals) const noexcept {
      auto count = individuals.size(),
           kth = static_cast<std::size_t>(std::sqrt(count)) + 1;

      auto first = distances.begin();
      for (std::size_t i = 0; i < count; ++i) {
        auto last = (first++) + count;

        std::ranges::nth_element(first, first + kth, last, std::ranges::less{});
        get_tag<crowd_density_t>(individuals[i]) = 1. / (*(first + kth) + 2.);

        first = last;
      }
    }
  };

  // cell-sharing (rdga, pesa, paes)
  template<double Alpha = 1.>
  class cluster {
  public:
    template<crowded_population Population, typename Preserved>
      requires(spatial_population<Population>)
    inline void
        operator()(Population& population,
                   population_pareto_t<Population, Preserved>& /*unused*/,
                   cluster_set const& clusters) const {
      for (auto&& individual : population.individuals()) {
        get_tag<crowd_density_t>(individual) =
            get_denisty(get_tag<cluster_label>(individual), clusters);
      }
    }

  private:
    inline auto get_denisty(cluster_label label,
                            cluster_set const& clusters) const {
      if (label.is_proper()) {
        auto count = static_cast<double>(clusters[label.index()].members_);
        return 1. - 1. / std::pow(count, Alpha);
      }

      return label.is_unique() ? 0. : 1.;
    }
  };

} // namespace crowd
} // namespace gal
