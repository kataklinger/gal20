
#pragma once

#include "utility.hpp"

namespace gal {
namespace cross {

  template<typename Chromosome>
  struct chromsome_init {
    using type = Chromosome;
    inline static auto prepare(type& target, type const& /*unused*/) noexcept {
      return std::back_inserter(target);
    }
  };

  template<typename Value, std::size_t Size>
  struct chromsome_init<std::array<Value, Size>> {
    using type = std::array<Value, Size>;
    inline static auto prepare(type& target, type const& /*unused*/) noexcept {
      return std::begin(target);
    }
  };

  template<typename Value, std::size_t Size>
  struct chromsome_init<Value[Size]> {
    using type = Value[Size];
    inline static auto prepare(type& target, type const& /*unused*/) noexcept {
      return std::begin(target);
    }
  };

  template<typename... Tys>
  struct chromsome_init<std::vector<Tys...>> {
    using type = std::vector<Tys...>;
    inline static auto prepare(type& target, type const& source) {
      target.reserve(source.size());
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

  } // namespace details

  template<typename Generator>
  class symmetric_singlepoint {
  public:
    using generator_t = Generator;

  private:
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline explicit symmetric_singlepoint(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    auto operator()(Chromosome const& parent1,
                    Chromosome const& parent2) const {
      using init = chromsome_init<Chromosome>;

      auto point = details::distribute(parent1, parent2)(*generator_);
      auto in1 = std::begin(parent1) + point, in2 = std::begin(parent2) + point;

      std::pair<Chromosome, Chromosome> children{};

      std::copy(in1,
                std::end(parent1),
                std::copy(std::begin(parent2),
                          in2,
                          init::prepare(children.first, parent1)));

      std::copy(in2,
                std::end(parent2),
                std::copy(std::begin(parent1),
                          in1,
                          init::prepare(children.second, parent2)));

      return children;
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator>
  class asymmetric_singlepoint {};

  template<typename Generator, std::size_t Points>
  class symmetric_multipoint {};

  template<typename Generator, std::size_t Points>
  class asymmetric_multipoint {};

} // namespace cross
} // namespace gal
