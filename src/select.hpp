#pragma once

#include <random>
#include <ranges>
#include <unordered_set>

#include "pop.hpp"

namespace gal {
namespace select {
  namespace details {

    template<std::size_t Size>
    struct nonunique_state {
      inline static constexpr std::size_t size = Size;
    };

    template<std::size_t Size>
    class unique_state {
    public:
      inline static constexpr std::size_t size = Size;

    public:
      inline unique_state() {
        existing_.reserve(Size);
      }

      inline void begin() noexcept {
        existing_.clear();
      }

      inline bool update(std::size_t selected) {
        return existing_.insert(selected).second;
      }

    private:
      std::unordered_set<std::size_t> existing_{};
    };

    template<typename Fn>
    concept index_producer = std::is_invocable_r_v<std::size_t, Fn>;

    template<std::size_t Size, index_producer Fn>
    std::size_t select_single(unique_state<Size>& s, Fn&& produce) {
      std::size_t idx{};
      do {
        idx = produce();
      } while (!s.update(idx));

      return idx;
    }

    template<std::size_t Size, index_producer Fn>
    std::size_t select_single(nonunique_state<Size> /*unused*/, Fn&& produce) {
      return produce();
    }

    template<typename Population, typename State, index_producer Fn>
    auto
        select_many(Population const& population, State&& state, Fn&& produce) {
      state.begin();

      std::array<typename Population::const_iterator_t, State::size> result{};
      std::ranges::generate(
          result.begin(),
          State::size,
          [first = population.individuals().begin(), &state, &produce] {
            return first + select_single(state, produce);
          });

      return result;
    }

    template<std::size_t Size, bool Unique>
    using state_t =
        std::conditional_t<Unique, unique_state<Size>, nonunique_state<Size>>;
  } // namespace details

  template<std::size_t Size, bool Unique, typename Generator>
  class pick_random {
  public:
    using generator_t = Generator;

  private:
    using state_t = details::state_t<Size, Unique>;
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit pick_random(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population const& population) {
      return details::select_many(
          population,
          state_,
          [dist = distribution_t{0, population.current_size() - 1},
           generator_]() { return dist(*generator_); });
    }

  private:
    generator_t* generator_;
    [[no_unique_address]] state_t state_{};
  };

  template<std::size_t Size, typename FitnessTag>
  class pick_best {
  private:
    using fitness_tag_t = FitnessTag;
    using state_t = details::nonunique_state<Size>;

  public:
    template<ordered_population<fitness_tag_t> Population>
    inline auto operator()(Population const& population) {
      population.sort(fitness_tag_t{});

      std::size_t idx{};
      return details::select_many(
          population, state_t{}, [&idx]() { return idx++; });
    }
  };

  template<std::size_t Size,
           bool Unique,
           typename FitnessTag,
           typename Generator>
  class pick_roulette {
  public:
    using generator_t = Generator;

  private:
    using fitness_tag_t = FitnessTag;
    using state_t = details::state_t<Size, Unique>;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline explicit pick_roulette(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population const& population) requires
        ordered_population<Population, FitnessTag> &&
        averageable_population<Population, FitnessTag> {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using distribution_t = random_fitness_distribution_t<fitness_t>;

      population.sort(fitness_tag);

      auto wheel = get_wheel(population);

      return details::select_many(
          population,
          state_,
          [&wheel, dist = distribution_t{{}, wheel.back()}, generator_]() {
            return std::ranges::lower_bound(wheel, dist(*generator_)) -
                   wheel.begin();
          });
    }

  private:
    template<typename Population>
    auto get_wheel(Population const& population) const {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;

      gal::stats::details::kahan_state<fitness_t> state{};
      auto wheel = population.individuals() |
                   std::ranges::views::transform([&state](auto& ind) {
                     state = state.add(ind.evaluation().get(fitness_tag));
                     return state.sum();
                   }) |
                   std::views::common;

      return std::vector<fitness_t>{wheel.begin(), wheel.end()};
    }

  private:
    generator_t* generator_;
    [[no_unique_address]] state_t state_{};
  };

} // namespace select
} // namespace gal