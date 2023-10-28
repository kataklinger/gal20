
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
    template<density_population<density_value_t> Population>
    void operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
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
      requires(density_population<Population, density_value_t> &&
               crowding_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population>& sets) {
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

          get_tag<density_tag>(**first) = get_tag<density_tag>(**last) =
              std::numeric_limits<double>::infinity();

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

  // average linkage (spea)
  class cluster {
  private:
    template<typename Individual>
    class rep {
    public:
      using individual_t = Individual;

    public:
      inline explicit rep(individual_t* individual)
          : members_{individual} {
      }

      inline void merge(rep const& merged) {
        members_.insert(
            members_.end(), merged.members_.begin(), merged.members_.end());
      }

      inline double distance(rep const& other) {
        double total_distance = 0.;

        for (auto const& outer : members_) {
          for (auto const& inner : other.members_) {
            total_distance +=
                distance(outer->evaluation().raw(), inner->evaluation().raw());
          }
        }

        return total_distance / (members_.size() * other.members_.size());
      }

      inline bool label_if_merged(std::size_t label) noexcept {
        if (members_.size() < 1) {
          return false;
        }

        for (auto&& member : members_) {
          get_tag<density_label_t>(member) = label;
        }

        return true;
      }

    private:
      template<typename Fitness>
      double distance(Fitness const& left, Fitness const& right) {
        double result = 0.;

        auto ir = std::ranges::begin(right);
        for (auto&& vl : left) {
          auto d = static_cast<double>(vl - *ir);
          result += d * d;

          ++ir;
        }

        return std::sqrt(result);
      }

    private:
      std::vector<individual_t*> members_;
    };

  public:
    template<typename Population>
      requires(density_population<Population, density_label_t> &&
               crowding_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
      using individual_t = typename Population::individual_t;

      clean_tags<density_label_t>(population);

      std::size_t populated_count = 0, cluster_label = 1;
      for (auto&& set : sets) {
        auto added_count = std::ranges::size(set);
        auto available_space =
            std::max(population.target_size() - populated_count, added_count);

        if (available_space < added_count) {
          auto clusters = generate_clusters(set);
          while (clusters.size() > available_space) {
            merge_closest(clusters);
          }

          for (auto&& c : clusters) {
            if (c.lable_if_merged(cluster_label)) {
              ++cluster_label;
            }
          }

          break;
        }

        populated_count += added_count;
      }
    }

  private:
    template<typename Set>
    static auto generate_clusters(Set& set) {
      std::vector<rep> culsters;
      culsters.reserve(std::ranges::size(set));

      for (auto&& individual : set) {
        culsters.emplace_back(individual);
      }

      return culsters;
    }

    template<typename Clusters>
    static void merge_closest(Clusters& clusters) {
      auto first = clusters.begin();

      auto min_distance = std::numeric_limits<double>::max();
      auto merge_left = first, merge_right = first;

      for (auto last = clusters.end(); first != end; ++first) {
        for (auto other = first + 1; other != end; ++other) {
          if (auto d = first->distance(*other); d < min_distance) {
            merge_left = first;
            merge_right = other;
          }
        }
      }

      merge_left->merge(*merge_right);
      clusters.erase(merge_right);
    }
  };

  // kth nearest neighbor (spea-ii)
  class neighbor {};

  // cell-sharing (pesa, pesa-ii, paes)
  class hypergrid {};

  // adaptive cell-sharing (rdga)
  class adaptive_hypergrid {};

} // namespace niche
} // namespace gal
