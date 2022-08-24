
#pragma once

#include "utility.hpp"

#include <tuple>

namespace gal {
namespace replace {

  namespace details {

    template<std::ranges::sized_range Replaced, typename Offspring>
    inline auto apply_replaced(Replaced& replaced, Offspring& offspring) {
      return std::views::iota(std::size_t{}, std::size(replaced)) |
             std::views::transform([&replaced, &offspring](std::size_t idx) {
               return gal::details::parentship{replaced[idx],
                                               get_child(offspring[idx])};
             });
    }

    template<typename Offspring>
    inline auto extract_children(Offspring& offspring) {
      return offspring |
             std::views::transform([](auto& o) { return get_child(o); });
    }

  } // namespace details

  template<std::size_t Size, typename Generator>
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
    inline auto operator()(Population& population, Offspring&& offspring) {
      auto to_replace = gal::details::select_many(
          population,
          state_,
          [dist = distribution_t{0, population.current_size() - 1}, this]() {
            return dist(*generator_);
          });

      return population.replace(details::apply_replaced(to_replace, offspring));
    }

  private:
    generator_t* generator_;
    gal::details::unique_state<Size> state_{};
  };

  template<std::size_t Size, typename FitnessTag>
  class worst {
  public:
    using fitness_tag_t = FitnessTag;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population, Offspring&& offspring) {
      population.sort(fitness_tag);

      std::size_t idx{population.current_size() - Size};
      auto to_replace =
          gal::details::select_many(population,
                                    gal::details::nonunique_state<Size>{},
                                    [&idx]() { return idx++; });

      return population.replace(details::apply_replaced(to_replace, offspring));
    }
  };

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
    inline auto operator()(Population& population, Offspring&& offspring) {

      population.insert(details::extract_children(offspring));
      population.sort(fitness_tag);
      return population.trim();
    }
  };

  template<std::size_t Size>
  class parents {
  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population, Offspring&& offspring) {
      return population.replace(offspring);
    }
  };

  class total {
  public:
    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population, Offspring&& offspring) {
      auto removed = population.trim_all();
      population.insert(details::extract_children(offspring));
      return removed;
    }
  };

} // namespace replace
} // namespace gal
