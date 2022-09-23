
#pragma once

#include "utility.hpp"

namespace gal {
namespace mutate {

  namespace details {

    template<std::ranges::sized_range Chromosome>
    inline auto get_count(std::size_t count,
                          Chromosome const& target) noexcept {
      return std::min(count, std::ranges::size(target));
    }

    template<std::ranges::sized_range Chromosome>
    inline auto get_count_min(std::size_t count,
                              Chromosome const& target) noexcept {
      if (auto size = std::ranges::size(target); size > 0) {
        return std::min(count, size);
      }

      return 0;
    }

    template<typename Generator, std::ranges::sized_range Chromosome>
    inline auto select(Generator& generator,
                       Chromosome const& chromosome) noexcept {
      return std::uniform_int_distribution<std::size_t>{
          0, std::ranges::size(chromosome) - 1}(generator);
    }

  } // namespace details

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class swap {};

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class move {};

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class destroy {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline explicit destroy(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    void operator()(Chromosome& target) {
      for (auto i = get_count_min(count, target); i > 0; --i) {
        auto it = std::begin(target);
        std::advance(it, details::select(*generator_, target));

        target.erase(it);
      }
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator, typename Fn, std::size_t Count>
  requires(Count > 0) class create {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline create(generator_t& generator, Fn fn)
        : generator_{&generator}
        , fn_{std::move(fn)} {
    }

    template<std::ranges::sized_range Chromosome>
    void operator()(Chromosome& target) {
      for (auto i = count; i > 0; --i) {
        auto it = std::begin(target);
        std::advance(it, details::select(*generator_, target));

        target.insert(it, fn_());
      }
    }

  private:
    generator_t* generator_;
    Fn fn_;
  };

  template<typename Generator, typename Fn, std::size_t Count>
  requires(Count > 0) class flip {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline flip(generator_t& generator, Fn fn)
        : generator_{&generator}
        , fn_{std::move(fn)} {
    }

    template<std::ranges::sized_range Chromosome>
    void operator()(Chromosome& target) {
      gal::details::unique_state state{details::get_count(count, target)};
      while (!state.full()) {
        std::size_t idx = 0;
        do {
          idx = details::select(*generator_, target);
        } while (!state.update(idx));

        auto it = std::begin(target);
        std::advance(it, idx);

        fn_(*it);
      }
    }

  private:
    generator_t* generator_;
    Fn fn_;
  };

  template<std::size_t Count, typename Generator, typename Flop>
  inline auto make_create(Generator& generator, Flop&& flop) {
    return create<Generator, std::remove_reference_t<Flop>, Count>{
        generator, std::forward<Flop>(flop)};
  }

  template<std::size_t Count, typename Generator, typename Flop>
  inline auto make_flip(Generator& generator, Flop&& flop) {
    return flip<Generator, std::remove_reference_t<Flop>, Count>{
        generator, std::forward<Flop>(flop)};
  }

} // namespace mutate
} // namespace gal
