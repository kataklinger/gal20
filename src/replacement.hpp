
#pragma once

#include "utility.hpp"

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
    inline auto translate(Replaced&& replaced, Offspring&& offspring) {
      return std::views::iota(std::size_t{}, std::size(replaced)) |
             std::views::transform([&replaced, &offspring](std::size_t idx) {
               return replacement_view{replaced[idx],
                                       get_child(offspring[idx])};
             });
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

      return population.replace(details::translate(to_replace, offspring));
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

      return population.replace(details::translate(to_replace, offspring));
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

      population.insert(offspring | std::views::transform(
                                        [](auto& o) { return get_child(o); }));
      population.sort(fitness_tag);
      return population.trim();
    }
  };

  template<std::size_t Size>
  class parents {};

  class total {};

} // namespace replace
} // namespace gal
