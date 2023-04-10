
#pragma once

#include "sampling.hpp"

#include <tuple>

namespace gal {
namespace replace {

  namespace details {

    template<typename Replaced, typename Replacement>
    struct replacement_view : std::tuple<Replaced, Replacement> {
      using std::tuple<Replaced, Replacement>::tuple;
    };

    template<typename Replaced, typename Replacement>
    replacement_view(Replaced&, Replacement&)
        -> replacement_view<Replaced&, Replacement&>;

    template<typename Replaced, typename Replacement>
    auto& get_parent(replacement_view<Replaced, Replacement>& pair) {
      return std::get<0>(pair);
    }

    template<typename Replaced, typename Replacement>
    auto& get_parent(replacement_view<Replaced, Replacement>&& pair) {
      return std::get<0>(pair);
    }

    template<typename Replaced, typename Replacement>
    auto& get_child(replacement_view<Replaced, Replacement>& pair) {
      return std::get<1>(pair);
    }

    template<typename Replaced, typename Replacement>
    auto& get_child(replacement_view<Replaced, Replacement>&& pair) {
      return std::get<1>(pair);
    }

    template<std::ranges::sized_range Replaced, typename Offspring>
    inline auto apply_replaced(Replaced& replaced, Offspring& offspring) {
      return std::views::iota(std::size_t{}, std::size(replaced)) |
             std::views::transform([&replaced, &offspring](std::size_t idx) {
               return replacement_view{replaced[idx],
                                       get_child(offspring[idx])};
             });
    }

    template<typename Offspring>
    inline auto extract_children(Offspring& offspring) {
      return offspring |
             std::views::transform([](auto& o) { return get_child(o); });
    }

  } // namespace details

  template<typename Generator, std::size_t Elitism>
  class random {
  public:
    using generator_t = Generator;

  private:
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit random(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      auto allowed = population.current_size() - Elitism;

      distribution_t dist{0, allowed - 1};
      auto to_replace = sample_many(
          population,
          unique_sample{std::min(allowed, std::ranges::size(offspring))},
          [this, &dist]() { return Elitism + dist(*generator_); });

      return population.replace(details::apply_replaced(to_replace, offspring));
    }

  private:
    generator_t* generator_;
  };

  template<typename FitnessTag>
  class worst {
  public:
    using fitness_tag_t = FitnessTag;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      population.sort(fitness_tag);

      auto removed = population.trim(std::ranges::size(offspring));
      population.insert(details::extract_children(offspring));
      return removed;
    }
  };

  using worst_raw = worst<raw_fitness_tag>;
  using worst_scaled = worst<raw_fitness_tag>;

  template<typename FitnessTag>
  class crowd {
  public:
    using fitness_tag_t = FitnessTag;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      population.insert(details::extract_children(offspring));
      population.sort(fitness_tag);
      return population.trim();
    }
  };

  using crowd_raw = crowd<raw_fitness_tag>;
  using crowd_scaled = crowd<raw_fitness_tag>;

  template<std::size_t Size>
  class parents {
  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      return population.replace(offspring);
    }
  };

  class total {
  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      auto removed = population.trim_all();
      population.insert(details::extract_children(offspring));
      return removed;
    }
  };

  class insert {
  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population,
                           Offspring&& offspring) const {
      population.insert(details::extract_children(offspring));
      return typename Population::collection_t{};
    }
  };

} // namespace replace
} // namespace gal
