
#pragma once

#include "utility.hpp"

namespace gal {
namespace cross {

  template<typename Chromosome>
  struct chromsome_init {
    using type = Chromosome;
    inline static auto prepare(type& target, std::size_t /*unused*/) noexcept {
      return std::back_inserter(target);
    }
  };

  template<typename Value, std::size_t Size>
  struct chromsome_init<std::array<Value, Size>> {
    using type = std::array<Value, Size>;
    inline static auto prepare(type& target, std::size_t /*unused*/) noexcept {
      return std::begin(target);
    }
  };

  template<typename Value, std::size_t Size>
  struct chromsome_init<Value[Size]> {
    using type = Value[Size];
    inline static auto prepare(type& target, std::size_t /*unused*/) noexcept {
      return std::begin(target);
    }
  };

  template<typename... Tys>
  struct chromsome_init<std::vector<Tys...>> {
    using type = std::vector<Tys...>;
    inline static auto prepare(type& target, std::size_t size) {
      target.reserve(size);
      return std::back_inserter(target);
    }
  };

  namespace details {

    using distribution_t = std::uniform_int_distribution<std::size_t>;

    template<std::ranges::sized_range Chromosome>
    inline auto distribute(Chromosome const& parent) noexcept {
      return distribution_t{1, std::ranges::size(parent) - 1};
    }

    template<std::ranges::sized_range Chromosome>
    inline auto distribute(Chromosome const& parent1,
                           Chromosome const& parent2) noexcept {
      return distribution_t{
          1,
          std::min(std::ranges::size(parent1), std::ranges::size(parent2)) - 1};
    }

    template<std::ranges::sized_range Chromosome>
    inline void splice(Chromosome& dest,
                       Chromosome const& left,
                       std::size_t point_left,
                       Chromosome const& right,
                       std::size_t point_right) {
      using init = chromsome_init<Chromosome>;

      auto size = point_left + std::ranges::size(right) - point_right;

      auto right_it = std::begin(right);
      std::advance(right_it, point_right);

      std::copy(
          right_it,
          std::end(right),
          std::copy_n(std::begin(left), point_left, init::prepare(dest, size)));
    }

  } // namespace details

  template<typename Generator>
  class symmetric_singlepoint {
  public:
    using generator_t = Generator;

  public:
    inline explicit symmetric_singlepoint(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    auto operator()(Chromosome const& p1, Chromosome const& p2) const {
      auto pt = details::distribute(p1, p2)(*generator_);

      std::pair<Chromosome, Chromosome> children{};

      details::splice(children.first, p1, pt, p2, pt);
      details::splice(children.second, p2, pt, p1, pt);

      return children;
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator>
  class asymmetric_singlepoint {
  public:
    using generator_t = Generator;

  public:
    inline explicit asymmetric_singlepoint(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    auto operator()(Chromosome const& p1, Chromosome const& p2) const {
      auto pt1 = details::distribute(p1)(*generator_),
           pt2 = details::distribute(p2)(*generator_);

      std::pair<Chromosome, Chromosome> children{};

      details::splice(children.first, p1, pt1, p2, pt2);
      details::splice(children.second, p2, pt2, p1, pt1);

      return children;
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator, std::size_t Points>
  class symmetric_multipoint {};

  template<typename Generator, std::size_t Points>
  class asymmetric_multipoint {};

} // namespace cross
} // namespace gal
