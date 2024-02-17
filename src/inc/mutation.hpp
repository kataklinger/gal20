
#pragma once

#include "sampling.hpp"

namespace gal::mutate {

namespace details {

  template<range_chromosome Chromosome>
  inline auto get_count(std::size_t count, Chromosome const& target) noexcept {
    return std::min(count, std::ranges::size(target));
  }

  template<range_chromosome Chromosome>
  inline std::size_t get_count_min(std::size_t count,
                                   Chromosome const& target) noexcept {
    if (auto size = std::ranges::size(target); size > 0) {
      return std::min(count, size);
    }

    return 0;
  }

  template<typename Generator, range_chromosome Chromosome>
  inline auto select(Generator& generator,
                     Chromosome const& chromosome) noexcept {
    return std::uniform_int_distribution<std::size_t>{
        0, std::ranges::size(chromosome) - 1}(generator);
  }

  template<range_chromosome Chromosome>
  inline auto get_iter(Chromosome& target, std::size_t index) noexcept {
    return std::pair{index,
                     std::ranges::next(std::ranges::begin(target), index)};
  }

  template<range_chromosome Chromosome, typename Generator>
  inline auto get_random_iter(Chromosome& target,
                              Generator& generator) noexcept {
    return get_iter(target, select(generator, target));
  }

} // namespace details

template<typename Generator, std::size_t Pairs>
  requires(Pairs > 0)
class interchange {
public:
  using generator_t = Generator;

  inline static constexpr auto pairs = Pairs;

public:
  inline explicit interchange(generator_t& generator,
                              countable_t<Pairs> /*unused*/)
      : generator_{&generator} {
  }

  template<range_chromosome Chromosome>
  void operator()(Chromosome& target) const {
    if (std::ranges::size(target) > 2) {
      for (auto i = pairs * 2; i > 0;) {
        auto [i1, left] = details::get_random_iter(target, *generator_);
        auto [i2, right] = details::get_random_iter(target, *generator_);

        if (left != right) {
          std::ranges::iter_swap(left, right);
          i -= 2;
        }
      }
    }
  }

private:
  generator_t* generator_;
};

template<typename Generator, std::size_t Count>
  requires(Count > 0)
class shuffle {
public:
  using generator_t = Generator;

  inline static constexpr auto count = Count;

public:
  inline explicit shuffle(generator_t& generator, countable_t<Count> /*unused*/)
      : generator_{&generator} {
  }

  template<range_chromosome Chromosome>
  void operator()(Chromosome& target) const {
    if (std::ranges::size(target) > 2) {
      for (auto i = count; i > 0;) {
        auto [idx_from, from] = details::get_random_iter(target, *generator_);
        auto [idx_to, to] = details::get_random_iter(target, *generator_);

        if (idx_from < idx_to) {
          std::ranges::rotate(
              from, std::ranges::next(from), std::ranges::next(to));
          --i;
        }
        else if (idx_from > idx_to) {
          std::ranges::rotate(to, from, std::ranges::next(from));
          --i;
        }
      }
    }
  }

private:
  generator_t* generator_;
};

template<typename Generator, std::size_t Count>
  requires(Count > 0)
class destroy {
public:
  using generator_t = Generator;

  inline static constexpr auto count = Count;

public:
  inline explicit destroy(generator_t& generator, countable_t<Count> /*unused*/)
      : generator_{&generator} {
  }

  template<range_chromosome Chromosome>
  void operator()(Chromosome& target) const {
    for (auto i = details::get_count_min(count, target); i > 0; --i) {
      target.erase(details::get_random_iter(target, *generator_).second);
    }
  }

private:
  generator_t* generator_;
};

template<typename Generator, std::size_t Count, typename Fn>
  requires(Count > 0)
class create {
public:
  using generator_t = Generator;

  inline static constexpr auto count = Count;

public:
  inline create(generator_t& generator, countable_t<Count> /*unused*/, Fn fn)
      : generator_{&generator}
      , fn_{std::move(fn)} {
  }

  template<range_chromosome Chromosome>
  void operator()(Chromosome& target) const {
    for (auto i = count; i > 0; --i) {
      target.insert(details::get_random_iter(target, *generator_).second,
                    fn_());
    }
  }

private:
  generator_t* generator_;
  Fn fn_;
};

template<typename Generator, std::size_t Count, typename Fn>
  requires(Count > 0)
class flip {
public:
  using generator_t = Generator;

  inline static constexpr auto count = Count;

public:
  inline flip(generator_t& generator, countable_t<Count> /*unused*/, Fn fn)
      : generator_{&generator}
      , fn_{std::move(fn)} {
  }

  template<range_chromosome Chromosome>
  void operator()(Chromosome& target) const {
    unique_sample state{details::get_count(count, target)};
    while (!state.full()) {
      std::size_t idx = 0;
      do {
        idx = details::select(*generator_, target);
      } while (!state.update(idx));

      fn_(*details::get_iter(target, idx).second);
    }
  }

private:
  generator_t* generator_;
  Fn fn_;
};

template<typename Generator, typename Distribution>
class roller {
public:
  using generator_t = Generator;
  using distribution_t = Distribution;

public:
  inline constexpr roller(generator_t& generator, distribution_t& distribution)
      : generator_{&generator}
      , distribution_{&distribution} {
  }

  template<typename Value>
  inline auto operator()(Value& value) const {
    value = (*this)();
  }

  inline auto operator()() const {
    return (*distribution_)(*generator_);
  }

private:
  generator_t* generator_;
  distribution_t* distribution_;
};

template<typename Generator, typename Distribution, std::size_t Count>
inline constexpr auto simple_create(Generator& generator,
                                    Distribution& distribution,
                                    countable_t<Count> count) {
  using roll_t = roller<Generator, Distribution>;
  return create{generator, count, roll_t{generator, distribution}};
}

template<typename Generator, typename Distribution, std::size_t Count>
inline constexpr auto simple_flip(Generator& generator,
                                  Distribution& distribution,
                                  countable_t<Count> count) {
  using roll_t = roller<Generator, Distribution>;
  return flip{generator, count, roll_t{generator, distribution}};
}

} // namespace gal::mutate
