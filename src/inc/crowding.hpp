
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

namespace gal {
namespace crowd {

  // fitness sharing (nsga)
  template<typename Proximity, double Cutoff, double Alpha = 2.>
  class sharing {
  public:
    using proximity_t = Proximity;

    inline static constexpr double cutoff = Cutoff;
    inline static constexpr double alpha = Alpha;

  public:
    inline sharing(proximity_t const& prox) noexcept(
        std::is_nothrow_copy_constructible_v<proximity_t>)
        : proximity_{prox} {
    }

    template<typename... Args>
      requires(std::is_constructible_v<proximity_t, Args...>)
    inline explicit sharing(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<proximity_t, Args...>)
        : proximity_{std::forward<Args>(args)...} {
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

        for (auto&& left : set) {
          for (auto&& right :
               std::ranges::subrange{std::ranges::next(std::ranges::begin(set)),
                                     std::ranges::end(set)}) {
            auto dist = static_cast<double>(
                proximity_(left->chromosome(), right->chromosome()));

            auto niching =
                dist < cutoff ? 1. - std::pow(dist / cutoff, alpha) : 0.;

            get_tag<crowd_density_t>(*left) += niching;
            get_tag<crowd_density_t>(*right) += niching;
            total += niching;
          }
        }

        for (auto&& individual : set) {
          auto& tag = get_tag<crowd_density_t>(*individual);
          tag += static_cast<double>(tag) / total;
        }
      }
    }

  private:
    proximity_t proximity_;
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

      for (auto&& set : sets) {
        for (std::size_t objective = 0; objective < objectives; ++objective) {
          std::ranges::sort(set, [objective](auto const& lhs, auto const& rhs) {
            return lhs->evaluation().raw()[objective] <
                   rhs->evaluation().raw()[objective];
          });

          auto first = std::ranges::begin(set);
          auto last = --std::ranges::end(set);

          get_tag<crowd_density_t>(**first) = get_tag<crowd_density_t>(**last) =
              std::numeric_limits<double>::infinity();

          while (++first != last) {
            auto distance = static_cast<double>(
                (*(first + 1))->evaluation().raw()[objective] -
                (*(first - 1))->evaluation().raw()[objective]);

            get_tag<crowd_density_t>(**first) += distance;
          }
        }

        auto max_distance = std::numeric_limits<double>::min();
        for (auto&& individual : set) {
          double distance{get_tag<crowd_density_t>(*individual)};
          max_distance = std::max(max_distance, distance);
        }

        for (auto&& individual : set) {
          auto& distance = get_tag<crowd_density_t>(*individual);
          distance = distance.get() / max_distance;
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
      auto count = individuals.size();

      auto kth = static_cast<std::size_t>(std::sqrt(count)) + 1;
      auto first = distances.begin(), last = distances.begin() + count;
      for (std::size_t i = 0; i < count; ++i) {

        auto d = *std::ranges::nth_element(
            first, first + kth, last, std::ranges::less{});
        get_tag<crowd_density_t>(individuals[i]) = 1. / (d + 2.);

        first = last;
        last += count;
      }
    }
  };

  // cell-sharing (pesa, pesa-ii, paes)
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
        return std::pow(count, Alpha);
      }

      if (label.is_unique()) {
        return 1.;
      }

      return 0.;
    }
  };

} // namespace crowd
} // namespace gal
