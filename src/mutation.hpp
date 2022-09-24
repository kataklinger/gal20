
#pragma once

#include "utility.hpp"

namespace gal {
namespace mutate {

  namespace details {

    template<range_chromosome Chromosome>
    inline auto get_count(std::size_t count,
                          Chromosome const& target) noexcept {
      return std::min(count, std::ranges::size(target));
    }

    template<range_chromosome Chromosome>
    inline auto get_count_min(std::size_t count,
                              Chromosome const& target) noexcept {
      if (auto size = std::ranges::size(target); size > 0) {
        return std::min(count, size);
      }

      return std::size_t{};
    }

    template<typename Generator, range_chromosome Chromosome>
    inline auto select(Generator& generator,
                       Chromosome const& chromosome) noexcept {
      return std::uniform_int_distribution<std::size_t>{
          0, std::ranges::size(chromosome) - 1}(generator);
    }

    template<range_chromosome Chromosome>
    inline auto get_iter(Chromosome& target, std::size_t index) noexcept {
      auto it = std::begin(target);
      std::advance(it, index);
      return it;
    }

    template<range_chromosome Chromosome, typename Generator>
    inline auto get_random_iter(Chromosome& target,
                                Generator& generator) noexcept {
      return get_iter(target, details::select(generator, target));
    }

  } // namespace details

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class interchange {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline explicit interchange(generator_t& generator)
        : generator_{&generator} {
    }

    template<range_chromosome Chromosome>
    void operator()(Chromosome& target) {
      if (std::ranges::size(target) > 2) {
        for (auto i = count; i > 0; --i) {
          auto left = details::get_random_iter(target, *generator_),
               right = details::get_random_iter(target, *generator_);

          if (left != right) {
            std::iter_swap(left, right);
            --i;
          }
        }
      }
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class shuffle {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline explicit shuffle(generator_t& generator)
        : generator_{&generator} {
    }

    template<range_chromosome Chromosome>
    void operator()(Chromosome& target) {
      if (std::ranges::size(target) > 2) {
        for (auto i = count; i > 0;) {
          auto from = details::get_random_iter(target, *generator_),
               to = details::get_random_iter(target, *generator_);

          if (from != to) {
            std::move(from, std::next(from), to);
            --i;
          }
        }
      }
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator, std::size_t Count>
  requires(Count > 0) class destroy {
  public:
    using generator_t = Generator;

    inline static constexpr auto count = Count;

  public:
    inline explicit destroy(generator_t& generator)
        : generator_{&generator} {
    }

    template<range_chromosome Chromosome>
    void operator()(Chromosome& target) {
      for (auto i = details::get_count_min(count, target); i > 0; --i) {
        target.erase(details::get_random_iter(target, *generator_));
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

    template<range_chromosome Chromosome>
    void operator()(Chromosome& target) {
      for (auto i = count; i > 0; --i) {
        target.insert(details::get_random_iter(target, *generator_), fn_());
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

    template<range_chromosome Chromosome>
    void operator()(Chromosome& target) {
      gal::details::unique_state state{details::get_count(count, target)};
      while (!state.full()) {
        std::size_t idx = 0;
        do {
          idx = details::select(*generator_, target);
        } while (!state.update(idx));

        fn_(*details::get_iter(target, idx));
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