
#pragma once

#include "utility.hpp"

namespace gal {
namespace mutate {

  namespace details {
    template<typename Generator, std::ranges::sized_range Chromosome>
    inline auto select(Generator& generator,
                       Chromosome const& chromosome) noexcept {
      return std::uniform_int_distribution<std::size_t>{
          0, std::ranges::size(chromosome) - 1}(generator);
    }

  } // namespace details

  template<typename Generator, std::size_t Count>
  class swap {};

  template<typename Generator, std::size_t Count>
  class move {};

  template<typename Generator, std::size_t Count>
  class destroy {};

  template<typename Generator, typename Flop, std::size_t Count>
  requires(Count > 0) class flip {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline flip(generator_t& generator, Flop const& flop)
        : generator_{&generator}
        , flop_{flop} {
    }

    inline flip(generator_t& generator, Flop&& flop)
        : generator_{&generator}
        , flop_{std::move(flop)} {
    }

    template<std::ranges::sized_range Chromosome>
    void operator()(Chromosome& chromosome) {
      gal::details::unique_state state{
          std::min(std::ranges::size(chromosome), count)};
      while (!state.full()) {
        std::size_t idx = 0;
        do {
          idx = details::select(*generator_, chromosome);
        } while (!state.update(idx));

        auto it = std::begin(chromosome);
        std::advance(it, idx);

        flop_(*it);
      }
    }

  private:
    generator_t* generator_;
    Flop flop_;
  };

  template<std::size_t Count, typename Generator, typename Flop>
  inline auto make_flip(Generator& generator, Flop&& flop) {
    return flip<Generator, std::remove_reference_t<Flop>, Count>{
        generator, std::forward<Flop>(flop)};
  }

} // namespace mutate
} // namespace gal