
#pragma once

#include "utility.hpp"

#include <tuple>

namespace gal {
namespace replace {

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

  template<std::size_t Size, typename Generator>
  class random {
  public:
    using generator_t = Generator;

  private:
    using state_t = details::unique_state<Size>;
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit random(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population,
             replacement_range<typename Population::iterator_t,
                               typename Population::individual_t> Offspring>
    inline auto operator()(Population& population, Offspring&& offspring) {
      auto child = std::begin(offspring);

      auto to_replace =
          details::select_many(
              population,
              state_,
              [dist = distribution_t{0, population.current_size() - 1},
               this]() { return dist(*generator_); }) |
          std::ranges::views::transform([&child](auto& replaced) {
            return replacement_view{replaced, get_child(*child++)};
          });

      return population.replace(to_replace);
    }

  private:
    generator_t* generator_;
    [[no_unique_address]] state_t state_{};
  };

  template<std::size_t Size, typename FitnessTag>
  class worst {};

  template<std::size_t Size, typename FitnessTag>
  class crowd {};

  template<std::size_t Size>
  class parents {};

  class total {};

} // namespace replace
} // namespace gal
