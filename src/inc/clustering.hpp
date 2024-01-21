
#pragma once

#include "multiobjective.hpp"

#include <unordered_map>

namespace gal {
namespace cluster {

  namespace details {

    template<typename Range>
    inline void assign_cluster_labels(Range&& range,
                                      cluster_label const& label) noexcept {
      for (auto&& member : range) {
        get_tag<cluster_label>(*member) = label;
      }
    }

  } // namespace details

  // no clustering (nsga, nsga-ii, spea-ii)
  class none {
  public:
    template<multiobjective_population Population, typename Preserved>
    inline auto operator()(
        Population& /*unused*/,
        population_pareto_t<Population, Preserved>& /*unused*/) const {
      return cluster_set{};
    }
  };

  // average linkage (spea)
  class linkage {
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
        std::size_t count = 0;

        for (auto const& outer : members_) {
          for (auto const& inner : other.members_) {
            total_distance += euclidean_distance(outer->evaluation().raw(),
                                                 inner->evaluation().raw());
            ++count;
          }
        }

        return total_distance / count;
      }

      inline void assign(cluster_set& clusters) noexcept {
        if (members_.size() == 1) {
          details::assign_cluster_labels(members_, cluster_label::unique());
        }
        else {
          auto label = clusters.add_cluster(members_.size());
          details::assign_cluster_labels(members_, label);
        }
      }

    private:
      std::vector<individual_t*> members_;
    };

  public:
    template<clustered_population Population, typename Preserved>
      requires(spatial_population<Population>)
    auto operator()(Population& population,
                    population_pareto_t<Population, Preserved>& sets) const {
      using individual_t = typename Population::individual_t;

      assert(population.target_size().has_value());

      cluster_set result;

      std::size_t filled = 0, target = *population.target_size();

      for (auto&& set : sets) {
        result.next_level();

        if (auto n = std::ranges::size(set); filled < target) {
          if (auto remain = std::min(target - filled, n); remain < n) {
            auto clusters = generate_clusters<individual_t>(set);
            while (clusters.size() > remain) {
              merge_closest(clusters);
            }

            for (auto&& c : clusters) {
              c.assign(result);
            }
          }
          else {
            details::assign_cluster_labels(set, cluster_label::unique());
          }

          filled += n;
        }
        else {
          details::assign_cluster_labels(set, cluster_label::unassigned());
        }
      }

      return result;
    }

  private:
    template<typename Individual, typename Set>
    static auto generate_clusters(Set& set) {
      std::vector<rep<Individual>> culsters;
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

      for (auto last = clusters.end(); first != last; ++first) {
        for (auto other = first + 1; other != last; ++other) {
          if (auto d = first->distance(*other); d < min_distance) {
            merge_left = first;
            merge_right = other;
            min_distance = d;
          }
        }
      }

      merge_left->merge(*merge_right);
      clusters.erase(merge_right);
    }
  };

  namespace details {

    template<typename Ty>
    struct hypercooridnate_element_impl {
      using type = Ty;
    };

    template<std::floating_point Ty>
    struct hypercooridnate_element_impl<Ty> {
      using type = std::ptrdiff_t;
    };

    template<typename Ty>
    using hypercooridnate_element_t =
        typename hypercooridnate_element_impl<Ty>::type;

    template<typename Ty, std::size_t N>
    using hypercooridnates_t = std::array<hypercooridnate_element_t<Ty>, N>;

    template<std::size_t I, typename Ty, std::size_t N>
    inline auto calculate_coord_hash(std::uint64_t value,
                                     std::array<Ty, N> const& coords) noexcept {
      constexpr std::uint64_t prime{0x100000001b3};
      if constexpr (I < N) {
        return calculate_coord_hash<I + 1>(
            value * prime ^ std::hash<Ty>{}(coords[I]), coords);
      }
      else {
        return value;
      }
    }

    class hypercooridnates_hash {
    public:
      template<typename Ty, std::size_t N>
      std::uint64_t operator()(std::array<Ty, N> const& coords) const noexcept {
        return calculate_coord_hash<0>(0xcbf29ce484222325, coords);
      }
    };

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
             typename Preserved,
             std::size_t Dimensions,
             typename FitnessValue = multiobjective_value_t<
                 get_fitness_t<raw_fitness_tag, Population>>>
    auto
        mark_hyperbox(Population& population,
                      population_pareto_t<Population, Preserved>& sets,
                      std::array<FitnessValue, Dimensions> const& granularity) {
      using individual_t = typename Population::individual_t;
      using hypermap_t =
          std::unordered_map<hypercooridnates_t<FitnessValue, Dimensions>,
                             std::vector<individual_t*>,
                             hypercooridnates_hash>;

      cluster_set result;

      std::size_t processed = true;
      for (auto&& set : sets) {
        if (processed < population.target_size()) {
          result.next_level();

          hypermap_t hypermap{};
          for (auto&& individual : set) {
            auto coords = get_hypercoordinates(individual->evaluation().raw(),
                                               granularity);
            hypermap[coords].push_back(individual);
          }

          processed += hypermap.size();

          for (auto&& [coord, individuals] : hypermap) {
            if (auto count = individuals.size(); count == 1) {
              get_tag<cluster_label>(*individuals[0]) = cluster_label::unique();
            }
            else {
              auto label = result.add_cluster(count);
              for (auto&& individual : individuals) {
                get_tag<cluster_label>(*individual) = label;
              }
            }
          }
        }
        else {
          for (auto&& individual : set) {
            get_tag<cluster_label>(*individual) = cluster_label::unassigned();
          }
        }
      }

      return result;
    }

  } // namespace details

  // hypercell (pesa, pesa-ii, paes)
  template<grid_fitness Fitness, multiobjective_value_t<Fitness>... Granularity>
  class hypergrid {
  private:
    inline static constexpr std::array<multiobjective_value_t<Fitness>,
                                       sizeof...(Granularity)>
        granularity{Granularity...};

  public:
    template<clustered_population Population, typename Preserved>
      requires(grid_population<Population>)
    inline auto
        operator()(Population& population,
                   population_pareto_t<Population, Preserved>& sets) const {
      return details::mark_hyperbox(population, sets, granularity);
    }
  };

  // adaptive cell-sharing (rdga)
  template<std::size_t... Divisions>
  class adaptive_hypergrid {
  private:
    inline static constexpr std::array<std::size_t, sizeof...(Divisions)>
        divisions{Divisions...};

  public:
    template<clustered_population Population, typename Preserved>
      requires(grid_population<Population>)
    auto operator()(Population& population,
                    population_pareto_t<Population, Preserved>& sets) const {
      using granularity_t =
          multiobjective_value_t<get_fitness_t<raw_fitness_tag, Population>>;

      auto first = population.individuals().begin();
      auto minimums = first->evaluation().raw();
      auto maximums = first->evaluation().raw();

      for (auto last = population.individuals().end(); first != last; ++first) {
        auto min_first = minimums.begin(), max_first = maximums.begin();
        for (auto&& v : first->evaluation().raw()) {
          if (v < *min_first) {
            *min_first = v;
          }
          else if (v > *max_first) {
            *max_first = v;
          }

          ++min_first;
          ++max_first;
        }
      }

      std::array<granularity_t, sizeof...(Divisions)> granularity{};

      auto gr_first = granularity.begin();
      auto div_first = divisions.begin();
      auto min_first = minimums.begin(), max_first = maximums.begin();
      for (auto last = minimums.end(); min_first != last;
           ++gr_first, ++div_first, ++min_first, ++max_first) {
        *gr_first =
            static_cast<granularity_t>((*max_first - *min_first) / *div_first);
      }

      return details::mark_hyperbox(population, sets, granularity);
    }
  };

} // namespace cluster
} // namespace gal
