
#pragma once

#include "utility.hpp"

#include <tuple>

namespace gal {
namespace replace {

  template<typename Replaced, typename Replacement>
  struct replacement_pair : std::tuple<Replaced, Replacement> {
      using std::tuple<Replaced, Replacement>::tuple;
  };

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
      auto replaced = details::select_many(
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
  class worst {};

  template<std::size_t Size, typename FitnessTag>
  class crowd {};

  template<std::size_t Size>
  class parents {};

  class total {};

} // namespace replace
} // namespace gal
