
#pragma once

#include "utility.hpp"

namespace gal {
namespace select {

  template<typename Attribute>
  concept attribute =
      requires {
        { Attribute::size } -> traits::decays_to<std::size_t>;
        { Attribute::unique } -> traits::decays_to<bool>;
        {
          Attribute::state()
          } -> std::same_as<details::state_t<Attribute::unique>>;
      };

  template<bool Unique, std::size_t Size>
  struct selection_attribute {
    inline static constexpr auto size = Size;
    inline static constexpr auto unique = Unique;

    inline static auto state() {
      return details::state_t<unique>{size};
    }
  };

  template<std::size_t Size>
  inline constexpr selection_attribute<false, Size> nonunique{};

  template<std::size_t Size>
  inline constexpr selection_attribute<true, Size> unique{};

  template<attribute Attribute, typename Generator>
  class random {
  public:
    using generator_t = Generator;
    using attribute_t = Attribute;

  private:
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit random(attribute_t /*unused*/,
                           generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) const {
      return details::select_many(
          population, attribute_t::state(), [&population, this]() {
            return distribution_t{0,
                                  population.current_size() - 1}(*generator_);
          });
    }

  private:
    generator_t* generator_;
  };

  template<std::size_t Size, typename FitnessTag>
  class best {
  private:
    using fitness_tag_t = FitnessTag;

  public:
    template<ordered_population<fitness_tag_t> Population>
    inline auto operator()(Population& population) const {
      population.sort(fitness_tag_t{});

      std::size_t idx{};
      return details::select_many(population,
                                  details::nonunique_state{Size},
                                  [&idx]() { return idx++; });
    }
  };

  template<std::size_t Size>
  using best_raw = best<Size, raw_fitness_tag>;

  template<std::size_t Size>
  using best_scaled = best<Size, scaled_fitness_tag>;

  template<typename FitnessTag, attribute Attribute, typename Generator>
  class roulette {
  public:
    using generator_t = Generator;
    using attribute_t = Attribute;

  private:
    using fitness_tag_t = FitnessTag;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline explicit roulette(attribute_t /*unused*/,
                             generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) const
      requires ordered_population<Population, FitnessTag> &&
               averageable_population<Population, FitnessTag>
    {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using distribution_t =
          typename fitness_traits<fitness_t>::random_distribution_t;

      population.sort(fitness_tag);

      auto wheel = get_wheel(population);

      return details::select_many(
          population, attribute_t::state(), [&wheel, this]() {
            auto selected = distribution_t{{}, wheel.back()}(*generator_);
            return std::ranges::lower_bound(wheel, selected) - wheel.begin();
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
  };

  template<attribute Attribute, typename Generator>
  class roulette_raw : public roulette<raw_fitness_tag, Attribute, Generator> {
  public:
    inline explicit roulette_raw(Attribute attrib,
                                 Generator& generator) noexcept
        : roulette<raw_fitness_tag, Attribute, Generator>{attrib, generator} {
    }
  };

  template<attribute Attribute, typename Generator>
  class roulette_scaled
      : public roulette<scaled_fitness_tag, Attribute, Generator> {
  public:
    inline explicit roulette_scaled(Attribute attrib,
                                    Generator& generator) noexcept
        : roulette<scaled_fitness_tag, Attribute, Generator>{attrib,
                                                             generator} {
    }
  };

} // namespace select
} // namespace gal
