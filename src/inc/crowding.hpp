
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

namespace gal {
namespace crowd {

  // fitness sharing (nsga)
  template<chromosome Chromosome,
           proximation<Chromosome> Proximation,
           double Cutoff,
           double Alpha = 2.>
  class sharing {
  public:
    using proximation_t = Proximation;

    inline constexpr double cutoff = Cutoff;
    inline constexpr double alpha = Alpha;

  public:
    inline explicit sharing(proximation_t proximity)
        : proximity_{proximity} {
    }

  public:
    template<crowded_population Population, typename Preserved>
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
            auto proximity = static_cast<double>(
                proximity_(left->chromosome(), right->chromosome()));

            auto niching = proximity < cutoff
                               ? 1. - std::pow(proximity / cutoff, alpha)
                               : 0.;

            get_tag<crowd_density_t>(*left) += niching;
            get_tag<crowd_density_t>(*right) += niching;
            total += niching
          }
        }

        for (auto&& individual : set) {
          auto& tag = get_tag<crowd_density_t>(*individual);
          tag += static_cast<double>(tag) / total;
        }
      }
    }

  private:
    proximation_t proximity_;
  };

  // crowding distance (nsga-ii)
  class distance {
  public:
    template<crowded_population Population, typename Preserved>
      requires(spatial_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population, Preserved>& sets,
                    cluster_set const& /*unused*/) const {
      clean_tags<crowd_density_t>(population);

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
          auto distance =
              static_cast<double>(get_tag<crowd_density_t>(*individual));
          max_distance = std::max(max_distance, distance);
        }

        for (auto&& individual : set) {
          auto& distance = get_tag<crowd_density_t>(*individual);
          distance = static_cast<double>(distance) / max_distance;
        }
      }
    }
  };

  // kth nearest neighbor (spea-ii)
  class neighbor {
  public:
    template<crowded_population Population, typename Preserved>
      requires(spatial_population<Population>)
    inline void operator()(Population& population,
                           population_pareto_t<Population, Preserved>& sets,
                           cluster_set const& /*unused*/) const {
      auto distances = compute_distances(population.individuals());
      assign_density(distances, population.individuals());
    }

  private:
    template<typename Individuals>
    std::vector<double> compute_distances(Individuals& individuals) {
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
    }

    template<typename Individuals>
    void assign_density(std::vector<double>& distances,
                        Individuals& individuals) noexcept {
      auto count = individuals.size();

      auto kth = static_cast<std::size_t>(std::sqrt(count)) + 1;
      auto first = distances.begin(), last = distances.begin() + count;
      for (std::size_t i = 0; i < count; ++i) {

        auto d =
            *std::ranges::nth_element(first, first + kth, last, std::less{});
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
    inline void operator()(Population& population,
                           population_pareto_t<Population, Preserved>& sets,
                           cluster_set const& clusters) const {
      for (auto&& individual : population.individuals()) {
        get_tag<crowd_density_t>(individual) =
            get_denisty(get_tag<cluster_label>(individual), clusters);
      }
    }

  private:
    inline auto get_denisty(cluster_label label, cluster_set const& clusters) {
      if (label.is_proper()) {
        auto count = static_cast<double>(sets[label.index()]);
        return std::pow(count, Alpha);
      }

      if (lable.is_unique()) {
        return 1.;
      }

      return 0.
    }
  };

} // namespace crowd
} // namespace gal
