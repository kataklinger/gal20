
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
        auto flhs = get_tag<RankTag>(lhs).get();
        auto frhs = get_tag<RankTag>(rhs).get();

        return flhs < frhs ||
               (flhs == frhs && get_tag<crowd_density_t>(lhs).get() <
                                    get_tag<crowd_density_t>(rhs).get());
      });

      population.trim();
    }
  };

  namespace details {

    template<typename Population>
    inline void sweep(Population& population) noexcept {
      population.remove_if([](auto const& individual) {
        return get_tag<prune_state_t>(individual).get();
      });
    }

    class cluster_map {
    private:
      inline static constexpr auto end_level =
          std::numeric_limits<std::size_t>::max();

    private:
      struct entry {
        entry() = default;

        inline entry(std::size_t index,
                     std::size_t level,
                     std::size_t first) noexcept
            : index_{index}
            , level_{level}
            , first_{first} {
        }

        inline auto prune_one() noexcept {
          return first_ + (pruned_++);
        }

        inline auto remaining() const noexcept {
          return count_ - pruned_;
        }

        std::size_t index_{};
        std::size_t level_{end_level};

        std::size_t first_{};

        std::size_t count_{};
        std::size_t pruned_{};
      };

    public:
      inline cluster_map(cluster_set const& clusters) {
        std::size_t buffer_size = 0, i = 0;
        for (auto&& cluster : clusters) {
          entries_.emplace_back(i++, cluster.level_, buffer_size);
          buffer_size += cluster.members_;
        }

        buffer_.resize(buffer_size);
      }

      inline void push(std::size_t cluster_index, prune_state_t& prune_state) {
        auto idx = entries_[cluster_index].count_++;
        buffer_[idx] = &prune_state;
      }

      inline void update_set(cluster_set& target) noexcept {
        for (auto&& entry : entries_) {
          target[entry.index_].members_ -= entry.pruned_;
        }
      }

    private:
      auto prepare(std::size_t excess) {
        auto proj = [](auto& s) { return std::tuple{s.level_, s.count_}; };

        auto relevant = std::min(excess, entries_.size());
        if (relevant < 8) {
          std::ranges::partial_sort(entries_,
                                    entries_.begin() + relevant,
                                    std::ranges::greater{},
                                    proj);
        }
        else {
          std::ranges::sort(entries_, std::ranges::greater{}, proj);
        }

        entries_.resize(relevant);
        entries_.emplace_back();
        next_ = entries_.begin();
      }

      inline auto more_levels() const noexcept {
        return next_->level_ != end_level;
      }

      template<typename Generator>
      auto fetch_level(Generator& generator) noexcept {
        auto current = next_;
        auto level = next_->level_;

        for (; next_->level_ == level && more_levels(); ++next_) {
          auto first = buffer_.begin() + next_->first_;
          std::shuffle(first, first + next_->count_, generator);
        }

        return std::tuple{
            current - entries_.begin(), current->remaining(), level};
      }

      inline void mark_one(std::size_t cluster_index) noexcept {
        *buffer_[entries_[cluster_index].prune_one()] = true;
      }

      inline auto same_density(std::size_t cluster_index,
                               std::size_t densest) const noexcept {
        return entries_[cluster_index].remaining() == densest;
      }

    public:
      template<typename Generator,
               std::invocable<std::size_t const&, std::size_t&> LevelPrune>
      void mark_all(Generator& generator,
                    LevelPrune level_prune,
                    std::size_t current_size,
                    std::size_t target_size) {
        if (current_size > target_size) {
          auto excess = current_size - target_size;
          prepare(excess);

          do {
            auto [i, density, level] = fetch_level(generator);
            for (; excess != 0 && density != 0; --density) {
              do {
                mark_one(i++);
              } while ((--excess) != 0 && same_density(i, density));
            }

            if (excess > 0) {
              level_prune(level, excess);
            }

          } while (excess != 0 && more_levels());
        }
      }

    private:
      std::vector<entry> entries_;
      std::vector<prune_state_t*> buffer_;

      std::vector<entry>::iterator next_;
    };

  } // namespace details

  // remove random from cluster (pesa, pesa-ii, paes)
  template<typename Generator>
  class cluster_random {
  public:
    using generator_t = Generator;

  public:
    inline explicit cluster_random(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<clustered_population Population>
      requires(prunable_population<Population>)
    void operator()(Population& population, cluster_set& clusters) const {
      assert(population.target_size().has_value());

      details::cluster_map map{clusters};

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

      auto level_prune = [&individuals = population.individuals()](
                             std::size_t level, std::size_t& excess) {
        for (auto&& individual : individuals) {
          if (get_tag<frontier_level_t>(individual) == level &&
              get_tag<cluster_label>(individual).is_unique()) {
            get_tag<prune_state_t>(individual) = true;

            if (--excess == 0) {
              break;
            }
          }
        }
      };

      map.mark_all(*generator_,
                   level_prune,
                   population.current_size() - unassigned,
                   *population.target_size());

      map.update_set(clusters);

      details::sweep(population);
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
      for (auto&& cluster : clusters) {
        buffer_index.emplace_back(buffer_size, 0);
        buffer_size += cluster.members_;
      }

      std::vector<std::tuple<individual_t*, double>> buffer(buffer_size);

      for (auto&& individual : population.individuals()) {
        if (auto label = get_tag<cluster_label>(individual);
            label.is_proper()) {
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

          if (current - first == clusters[label.index()].members_) {
            auto& [center, dummy] = *std::ranges::min_element(
                buffer.begin() + first,
                buffer.begin() + current,
                std::ranges::less{},
                [](auto& element) { std::get<1>(element); });

            get_tag<prune_state_t>(center) = false;
          }
        }
        else {
          get_tag<prune_state_t>(individual) = !label.is_unique();
        }
      }

      for (auto&& cluster : clusters) {
        cluster.members_ = 1;
      }

      details::sweep(population);
    }
  };

} // namespace prune
} // namespace gal
