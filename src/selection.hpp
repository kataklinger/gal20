
#pragma once

#include "utility.hpp"

namespace gal {
namespace select {

  template<std::size_t Size, bool Unique, typename Generator>
  class random {
  public:
    using generator_t = Generator;

  private:
    using state_t = details::state_t<Unique>;
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit random(generator_t& generator) noexcept
        : generator_{&generator}
        , state_{Size} {
    }

    template<typename Population>
    inline auto operator()(Population& population) {
      return details::select_many(
          population,
          std::false_type{},
          state_,
          [dist = distribution_t{0, population.current_size() - 1}, this]() {
            return dist(*generator_);
          });
    }

  private:
    generator_t* generator_;
    [[no_unique_address]] state_t state_{};
  };

  template<std::size_t Size, typename FitnessTag>
  class best {
  private:
    using fitness_tag_t = FitnessTag;

  public:
    template<ordered_population<fitness_tag_t> Population>
    inline auto operator()(Population& population) {
      population.sort(fitness_tag_t{});

      std::size_t idx{};
      return details::select_many(population,
                                  std::false_type{},
                                  details::nonunique_state{Size},
                                  [&idx]() { return idx++; });
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
    using state_t = details::state_t<Unique>;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline explicit roulette(generator_t& generator) noexcept
        : generator_{&generator}
        , state_{Size} {
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
          std::false_type{},
          state_,
          [&wheel, dist = distribution_t{{}, wheel.back()}, this]() {
            return std::ranges::lower_bound(wheel, dist(*generator_)) -
                   wheel.begin();
          });
    }

  private:
    template<typename Population>
    auto get_wheel(Population& population) const {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

      state_t state{};
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
    [[no_unique_address]] state_t state_;
  };

} // namespace select
} // namespace gal
