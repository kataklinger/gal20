
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
  private:
    class cluster_map {
    private:
      struct entry {
        inline entry() noexcept
            : count_{std::numeric_limits<std::size_t>::max()} {
        }

        inline entry(std::size_t index, std::size_t first) noexcept
            : index_{index}
            , first_{first} {
        }

        inline auto prune_one() noexcept {
          return first_ + (pruned_++);
        }

        inline auto remaining() const noexcept {
          return count_ - pruned_;
        }

        std::size_t index_{};
        std::size_t first_{};
        std::size_t count_{};
        std::size_t pruned_{};
      };

    public:
      inline cluster_map(cluster_set& clusters) {
        std::size_t buffer_size = 0, i = 0;
        for (auto&& cluster_size : clusters) {
          states.emplace_back(i++, buffer_size);
          buffer_size += cluster_size;
        }

        buffer_.resize(buffer_size);
      }

      inline void push(std::size_t cluster_index, prune_state_t& prune_state) {
        auto idx = entries_[cluster_index].count_++;
        buffer_[idx] = &prune_state;
      }

      auto prepare(std::size_t prepruned, std::size_t target_size) {
        auto prune_count = prepruned - target_size;
        auto relevant = std::min(prune_count, entries_.size());

        if (auto proj = [](auto& s) { return s.count_; }; relevant < 8) {
          std::ranges::partial_sort(
              entries_, entries_.begin() + relevant, std::greater{}, proj);
        }
        else {
          std::ranges::sort(entries_, std::greater{}, proj);
        }

        entries_.resize(relevant);
        entries_.emplace_back();

        for (auto&& entry : entries_) {
          auto first = buffer.begin() + entry.first_;
          std::shuffle(first, first + entry.count_, generator_);
        }

        return std::tuple{prune_count, entries_[0].remaining()};
      }

      inline void mark(std::size_t cluster_index) noexcept {
        *buffer_[entries_[cluster_index].prune_one()] = true;
      }

      inline auto more(std::size_t cluster_index,
                       std::size_t densest) const noexcept {
        return entries_[cluster_index].remaining() == densest;
      }

      inline void update_set(cluster_set& target) noexcept {
        for (auto&& entry : entries_) {
          target[entry.index_] -= entry.pruned_;
        }
      }

    private:
      std::vector<entry> entries_;
      std::vector<prune_state_t*> buffer_;
    };

  public:
    using generator_t = Generator;

  public:
    inline explicit cluster_random(generator_t& generator) noexcept
        : generator_{generator} {
    }

    template<clustered_population Population>
      requires(prunable_population<Population>)
    void operator()(Population& population, cluster_set& clusters) const {
      cluster_map map{clusters};

      std::size_t unassigned = 0;
      for (auto&& individual : population.individuals()) {
        auto label = get_tag<cluster_label>(individual);
        if (label.is_unassigned()) {
          get_tag<prune_state_t>(individual) = true;
          ++unassigned;
        }
        else if (label.is_proper()) {
          map.push(label.index(), get_tag<prune_state_t>(individual));
        }
      }

      auto reduced = population.current_size() - unassigned;
      if (auto target = population.target_size(); reduced > target) {
        auto [excess, densest] = map.prepare(reduced, target);
        for (std::size_t i = 0; excess != 0 && densest != 0; --densest) {
          do {
            map.mark(i++);
          } while ((--excess) != 0 && map.more(i, densest));
        }
      }

      map.update_set(clusters);
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
