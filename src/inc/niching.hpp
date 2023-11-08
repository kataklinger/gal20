
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

#include <cmath>
#include <unordered_set>

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

  namespace details {

    template<typename Range>
    inline void assign_density_data(Range&& range,
                                    std::size_t label,
                                    double density) noexcept {
      for (auto&& member : range) {
        get_tag<density_label_t>(member) = label;
        get_tag<density_value_t>(member) = density;
      }
    }

  } // namespace details

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
            total_distance += euclidean_distance(outer->evaluation().raw(),
                                                 inner->evaluation().raw());
          }
        }

        return total_distance / (members_.size() * other.members_.size());
      }

      inline bool assign(std::size_t label) noexcept {
        if (members_.size() == 1) {
          details::assign_density_data(members_, 0, 1.);
          return true;
        }

        details::assign_density_data(members_, label, 1. / members_.size());
        return false;
      }

    private:
      std::vector<individual_t*> members_;
    };

  public:
    template<typename Population>
      requires(density_population<Population, density_value_t> &&
               density_population<Population, density_label_t> &&
               crowding_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
      std::size_t filled = 0, taget = population.target_size(),
                  cluster_label = 1;
      for (auto&& set : sets) {
        if (auto n = std::ranges::size(set); filled < taget) {
          if (auto remain = std::max(taget - filled, n); remain < n) {
            auto clusters = generate_clusters(set);
            while (clusters.size() > remain) {
              merge_closest(clusters);
            }

            for (auto&& c : clusters) {
              if (c.assign(cluster_label)) {
                ++cluster_label;
              }
            }
          }
          else {
            details::assign_density_data(set, 0, 1.);
          }

          filled += n;
        }
        else {
          details::assign_density_data(set, cluster_label, 1.);
        }
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
  class neighbor {
  public:
    template<typename Population>
      requires(density_population<Population, density_value_t> &&
               crowding_population<Population>)
    inline void operator()(Population& population,
                           population_pareto_t<Population>& sets) {
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
        get_tag<density_value_t>(individuals[i]) = 1. / (d + 2.);

        first = last;
        last += count;
      }
    }
  };

  namespace details {

    template<typename Ty>
    struct hypercooridnate_element_impl {
      using type = Ty;
    };

    template<std::floating_point Ty>
    struct hypercooridnate_element_impl {
      using type = std::ptrdiff_t;
    };

    template<typename Ty>
    using hypercooridnate_element_t =
        typename hypercooridnate_element_impl<Ty>::type;

    template<typename Ty, std::size_t>
    struct hypercooridnates_map_element {
      using type = Ty;
    };

    template<typename Ty, typename Idxs>
    struct hypercooridnates_impl;

    template<typename Ty, std ::size_t... Idxs>
    struct hypercooridnates_impl<Ty, std::index_sequence<Idxs...>> {
      using type =
          std::tuple<typename hypercooridnates_map_element<Ty, Idxs>::type...>;
    };

    template<typename BaseType, std::size_t Dimensions>
    using hypercooridnates_t = typename hypercooridnates_impl<
        hypercooridnate_element_t<BaseType>,
        std::make_index_sequence<Dimensions>>::type;

    template<typename Value>
    inline Value cacluate_hypercoordinate(Value value, Value size) noexcept {
      constexpr Value zero{0}, neg_1{-1};
      return value >= zero ? value / size : neg_1 - (neg_1 - value) / size;
    }

    template<std::unsigned_integral Value>
    inline Value cacluate_hypercoordinate(Value value, Value size) noexcept {
      return value / size;
    }

    template<std::floating_point Value>
    inline std::ptrdiff_t cacluate_hypercoordinate(Value value,
                                                   Value size) noexcept {
      return static_cast<std::ptrdiff_t>(std::floor(value / size));
    }

    template<grid_fitness Fitness, std::size_t... Idxs>
    inline auto
        get_hypercoordinates(Fitness const& fitness,
                             std::array<multiobjective_value_t<Fitness>,
                                        sizeof...(Idxs)> const& granularity,
                             std::index_sequence<Idxs...> /*unused*/) noexcept {
      return details::hypercooridnates_t<multiobjective_value_t<Fitness>,
                                         sizeof...(Idxs)>{
          cacluate_hypercoordinate(fitness[Idxs], granularity[Idxs])...};
    }

    template<grid_fitness Fitness, std::size_t Dimensions>
    inline auto get_hypercoordinates(
        Fitness const& fitness,
        std::array<multiobjective_value_t<Fitness>, Dimensions> const&
            granularity) noexcept {
      return get_hypercoordinates<Fitness>(
          fitness, granularity, std::make_index_sequence<Dimensions>{});
    }

    template<typename Population,
             std::size_t Dimensions,
             typename Fitness = get_fitness_t<raw_fitness_tag, Population>>
    void calculate_hyperbox_density(
        Population& population,
        population_pareto_t<Population>& sets,
        std::array<Fitness, Dimensions> const& granularity,
        double alpha) {
      using label_set_t = std::unordered_set<
          hypercooridnates_t<multiobjective_value_t<Fitness>, Dimensions>>;

      std::vector<std::size_t> densities{};
      densities.reserve(population.current_size());

      for (auto&& set : sets) {
        label_set_t labels{};

        for (auto&& individual : set) {
          auto hyperbox =
              get_hypercoordinates(individual.evaluation().raw(), granularity);

          if (auto [it, added] = labels.try_emplace(hyperbox); added) {
            densities.push_back(1);
          }
          else {
            *densities.rbegin() += 1;
          }

          get_tag<density_label_t>(individual) = densities.size() - 1;
        }
      }

      for (auto&& individual : population.individuals()) {
        auto label = get_tag<density_label_t>(individual);
        auto density = static_cast<double>(densities[label]);

        get_tag<density_value_t>(individual) = 1. / std::pow(density, alpha);
      }
    }

  } // namespace details

  // cell-sharing (pesa, pesa-ii, paes)
  template<grid_fitness Fitness,
           double Alpha = 2.0,
           multiobjective_value_t<Fitness>... Granularity>
  class hypergrid {
  private:
    inline static constexpr std::array<multiobjective_value_t<Fitness>,
                                       sizeof...(Granularity)>
        granularity{Granularity...};

  public:
    template<typename Population>
      requires(density_population<Population, density_value_t> &&
               density_population<Population, density_label_t> &&
               grid_population<Population>)
    inline void operator()(Population& population,
                           population_pareto_t<Population>& sets) const {
      details::calculate_hyperbox_density(population, sets, granularity, Alpha);
    }
  };

  // adaptive cell-sharing (rdga)
  template<double Alpha = 2.0, std::size_t... Divisions>
  class adaptive_hypergrid {
  public:
    template<typename Population>
      requires(density_population<Population, density_value_t> &&
               density_population<Population, density_label_t> &&
               grid_population<Population>)
    void operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
      // calculate granularity for each dimension:
      //   (max[f.x] - min[f.x]) / Div[x]
      //
      // details::calculate_hyperbox_density(
      //   population, sets, granularity, Alpha);
    }
  };

} // namespace niche
} // namespace gal
