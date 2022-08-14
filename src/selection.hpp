
#pragma once

#include "utility.hpp"

namespace gal {
namespace select {

  template<std::size_t Size, bool Unique, typename Generator>
  class random {
  public:
    using generator_t = Generator;

  private:
    using state_t = details::state_t<Size, Unique>;
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit random(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) {
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
  class best {
  private:
    using fitness_tag_t = FitnessTag;
    using state_t = details::nonunique_state<Size>;

  public:
    template<ordered_population<fitness_tag_t> Population>
    inline auto operator()(Population& population) {
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
  class roulette {
  public:
    using generator_t = Generator;

  private:
    using fitness_tag_t = FitnessTag;
    using state_t = details::state_t<Size, Unique>;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline explicit roulette(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) requires
        ordered_population<Population, FitnessTag> &&
        averageable_population<Population, FitnessTag> {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using distribution_t =
          typename fitness_traits<fitness_t>::random_distribution_t;

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
    auto get_wheel(Population& population) const {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

      auto wheel =
          population.individuals() |
          std::ranges::views::transform([state = state_t{}](auto& ind) {
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
