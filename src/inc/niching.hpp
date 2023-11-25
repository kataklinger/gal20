
#pragma once

#include "multiobjective.hpp"
#include "operation.hpp"

#include <cmath>
#include <unordered_map>

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
    template<niched_population Population>
    auto operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
      clean_tags<niche_density_t>(population);

      niche_set niches;

      for (auto&& set : sets) {
        double total = 0.;

        auto& label =
            niches.add_front().add_niche(std::ranges::size(set)).label();

        for (auto&& left : set) {
          get_tag<niche_label>(*left) = label;

          for (auto&& right :
               std::ranges::subrange{std::ranges::next(std::ranges::begin(set)),
                                     std::ranges::end(set)}) {
            auto proximity = static_cast<double>(
                proximity_(left->chromosome(), right->chromosome()));

            auto niching = proximity < cutoff
                               ? 1. - std::pow(proximity / cutoff, alpha)
                               : 0.;

            get_tag<niche_density_t>(*left) += niching;
            get_tag<niche_density_t>(*right) += niching;
            total += niching
          }
        }

        for (auto&& inidividual : set) {
          auto& tag = get_tag<niche_density_t>(*inidividual);
          tag += static_cast<double>(tag) / total;
        }
      }

      return niches;
    }

  private:
    proximation_t proximity_;
  };

  // crowding distance (nsga-ii)
  class distance {
  public:
    template<niched_population Population>
      requires(crowding_population<Population>)
    auto operator()(Population& population,
                    population_pareto_t<Population>& sets) {
      clean_tags<niche_density_t>(population);

      auto objectives =
          std::ranges::size(population.individuals().evaluation().raw());

      niche_set niches;

      for (auto&& set : sets) {
        for (std::size_t objective = 0; objective < objectives; ++objective) {
          std::ranges::sort(set, [objective](auto const& lhs, auto const& rhs) {
            return lhs->evaluation().raw()[objective] <
                   rhs->evaluation().raw()[objective];
          });

          auto first = std::ranges::begin(set);
          auto last = --std::ranges::end(set);

          get_tag<niche_density_t>(**first) = get_tag<niche_density_t>(**last) =
              std::numeric_limits<double>::infinity();

          while (++first != last) {
            auto distance = static_cast<double>(
                (*(first + 1))->evaluation().raw()[objective] -
                (*(first - 1))->evaluation().raw()[objective]);

            get_tag<niche_density_t>(**first) += distance;
          }
        }

        auto& label =
            niches.add_front().add_niche(std::ranges::size(set)).label();

        auto max_distance = std::numeric_limits<double>::min();
        for (auto&& inidividual : set) {
          get_tag<niche_label>(*inidividual) = label;

          auto distance =
              static_cast<double>(get_tag<niche_density_t>(*inidividual));
          max_distance = std::max(max_distance, distance);
        }

        for (auto&& inidividual : set) {
          auto& distance = get_tag<niche_density_t>(*inidividual);
          distance = static_cast<double>(distance) / max_distance;
        }
      }

      return niches;
    }
  };

  namespace details {

    template<typename Range>
    inline void assign_density_data(Range&& range,
                                    niche_label const& label,
                                    double density) noexcept {
      for (auto&& member : range) {
        get_tag<niche_label>(member) = label;
        get_tag<niche_density_t>(member) = density;
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

      inline void assign(niche_set& niches) noexcept {
        if (members_.size() == 1) {
          details::assign_density_data(members_, niches.unique_label(), 1.);
        }
        else {
          auto& label = niches.add_niche(members_.size()).label();
          details::assign_density_data(members_, label, 1. / members_.size());
        }
      }

    private:
      std::vector<individual_t*> members_;
    };

  public:
    template<niched_population Population>
      requires(crowding_population<Population>)
    auto operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
      std::size_t filled = 0, target = population.target_size();

      niche_set niches;

      for (auto&& set : sets) {
        niches.add_front();

        if (auto n = std::ranges::size(set); filled < target) {
          if (auto remain = std::max(target - filled, n); remain < n) {
            auto clusters = generate_clusters(set);
            while (clusters.size() > remain) {
              merge_closest(clusters);
            }

            for (auto&& c : clusters) {
              c.assign(niches);
            }
          }
          else {
            details::assign_density_data(set, niches.unique_label(), 1.);
          }

          filled += n;
        }
        else {
          details::assign_density_data(set, {}, 1.);
        }
      }

      return niches;
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
    template<niched_population Population>
      requires(crowding_population<Population>)
    inline auto operator()(Population& population,
                           population_pareto_t<Population>& sets) {
      niche_set niches;

      auto distances = compute_distances(population.individuals());
      assign_density(distances, population.individuals());

      for (auto&& set : sets) {
        auto& label =
            niches.add_front().add_niche(std::ranges::size(set)).label();

        for (auto&& individual : set) {
          get_tag<niche_label>(*individual) = label;
        }
      }

      return niches;
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
        get_tag<niche_density_t>(individuals[i]) = 1. / (d + 2.);

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
    auto calculate_hyperbox_density(
        Population& population,
        population_pareto_t<Population>& sets,
        std::array<Fitness, Dimensions> const& granularity,
        double alpha) {
      using individual_t = typename Population::individual_t;
      using hypermap_t = std::unordered_map<
          hypercooridnates_t<multiobjective_value_t<Fitness>, Dimensions>,
          std::vector<individual_t*>>;

      niche_set niches;

      for (auto&& set : sets) {
        niches.add_front();

        hypermap_t hypermap{};
        for (auto&& individual : set) {
          auto coords =
              get_hypercoordinates(individual.evaluation().raw(), granularity);

          hypermap[coords].push_back(&individual);
        }

        for (auto&& [coord, individuals] : hypermap) {
          if (auto count = individuals.size(); count == 1) {
            get_tag<niche_label>(*individuals[0]) = niches.unique_label();
            get_tag<niche_density_t>(*individuals[0]) = 1.;
          }
          else {
            auto& label = niches.add_niche(count).label();
            auto density = 1. / std::pow(count, alpha);

            for (auto&& individual : individuals) {
              get_tag<niche_label>(*individual) = label;
              get_tag<niche_density_t>(*individual) = density;
            }
          }
        }
      }

      return niches;
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
    template<niched_population Population>
      requires(grid_population<Population>)
    inline auto operator()(Population& population,
                           population_pareto_t<Population>& sets) const {
      return details::calculate_hyperbox_density(
          population, sets, granularity, Alpha);
    }
  };

  // adaptive cell-sharing (rdga)
  template<double Alpha = 2.0, std::size_t... Divisions>
  class adaptive_hypergrid {
  private
    inline static constexpr std::array<std::size_t, sizeof...(Divisions)>
        divisions{Divisions...};

  public:
    template<niched_population Population>
      requires(grid_population<Population>)
    auto operator()(Population& population,
                    population_pareto_t<Population>& sets) const {
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
        *gr_first = (*max_first - *min_first) / *div_first;
      }

      return details::calculate_hyperbox_density(
          population, sets, granularity, Alpha);
    }
  };

} // namespace niche
} // namespace gal
