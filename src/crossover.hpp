
#pragma once

#include "utility.hpp"

namespace gal {
namespace cross {

  namespace details {

    template<typename Chromosome>
    struct chromsome_init {
      using type = Chromosome;
      inline static auto prepare(type& target,
                                 std::size_t /*unused*/) noexcept {
        return std::back_inserter(target);
      }
    };

    template<typename Value, std::size_t Size>
    struct chromsome_init<std::array<Value, Size>> {
      using type = std::array<Value, Size>;
      inline static auto prepare(type& target,
                                 std::size_t /*unused*/) noexcept {
        return std::begin(target);
      }
    };

    template<typename Value, std::size_t Size>
    struct chromsome_init<Value[Size]> {
      using type = Value[Size];
      inline static auto prepare(type& target,
                                 std::size_t /*unused*/) noexcept {
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

  } // namespace details

  template<std::ranges::sized_range Chromosome>
  inline auto prepare_chromosome(Chromosome& chromosome, std::size_t size) {
    return details::chromsome_init<Chromosome>::prepare(chromosome, size);
  }

  namespace details {

    using distribution_t = std::uniform_int_distribution<std::size_t>;

    template<std::ranges::sized_range Chromosome>
    using get_iterator_type = std::remove_reference_t<decltype(std::begin(
        std::declval<std::add_lvalue_reference_t<Chromosome>>()))>;

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
      auto size = point_left + std::ranges::size(right) - point_right;

      auto right_it = std::begin(right);
      std::advance(right_it, point_right);

      std::copy(right_it,
                std::end(right),
                std::copy_n(std::begin(left),
                            point_left,
                            prepare_chromosome(dest, size)));
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
    inline auto operator()(Chromosome const& p1, Chromosome const& p2) const {
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
    inline auto operator()(Chromosome const& p1, Chromosome const& p2) const {
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
  requires(Points > 1) class symmetric_multipoint {
  public:
    using generator_t = Generator;

    inline static constexpr auto points = Points;

  public:
    inline explicit symmetric_multipoint(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    auto operator()(Chromosome const& p1, Chromosome const& p2) const {
      auto size1 = std::ranges::size(p1), size2 = std::ranges::size(p2);
      auto count = std::min({points, size1 - 1, size2 - 1});

      if (count == 0) {
        return std::pair<Chromosome, Chromosome>{p1, p2};
      }

      auto selected = gal::details::selecte_many(
          gal::details::unique_state{count}, [&p1, &p2, this] {
            return details::distribute(p1, p2)(*generator_);
          });

      std::ranges::sort(selected);

      if (selected.size() % 2 == 1) {
        std::swap(size1, size2);
      }

      std::pair<Chromosome, Chromosome> children{};
      auto out1 = prepare_chromosome(children.first, size1),
           out2 = prepare_chromosome(children.second, size2);

      splice(p1, p2, selected, out1, out2);

      return children;
    }

  private:
    template<std::ranges::random_access_range Chromosome, typename It>
    void splice(Chromosome const& p1,
                Chromosome const& p2,
                std::vector<std::size_t> const& selected,
                It out1,
                It out2) const {
      std::pair in{std::begin(p1), std::begin(p2)};
      for (auto point : selected) {
        std::pair next{std::begin(p1) + point, std::begin(p2) + point};

        out1 = std::copy(in.first, next.first, out1);
        out2 = std::copy(in.second, next.second, out2);

        in = next;
        std::swap(out1, out2);
      }

      std::copy(in.first, std::end(p1), out1);
      std::copy(in.second, std::end(p2), out2);
    }

    template<std::ranges::forward_range Chromosome, typename It>
    auto splice(Chromosome const& p1,
                Chromosome const& p2,
                std::vector<std::size_t> const& selected,
                It out1,
                It out2) const {
      auto in1 = std::begin(p1), in2 = std::begin(p2);

      std::size_t current = 0;
      for (auto point : selected) {
        for (; current < point; ++current) {
          *(out1++) = *(in1++);
          *(out2++) = *(in2++);
        }

        std::swap(out1, out2);
      }

      std::copy(in1, std::end(p1), out1);
      std::copy(in2, std::end(p2), out2);
    }

  private:
    generator_t* generator_;
  };

  template<typename Generator, std::size_t Points>
  requires(Points > 1) class asymmetric_multipoint {
  public:
    using generator_t = Generator;

    inline static constexpr auto points = Points;

  public:
    inline explicit asymmetric_multipoint(generator_t& generator)
        : generator_{&generator} {
    }

    template<std::ranges::sized_range Chromosome>
    auto operator()(Chromosome const& p1, Chromosome const& p2) const {
      auto size1 = std::ranges::size(p1), size2 = std::ranges::size(p2);
      auto count = std::min({points, size1 - 1, size2 - 1});

      if (count == 0) {
        return std::pair<Chromosome, Chromosome>{p1, p2};
      }

      auto selected1 = gal::details::selecte_many(
          gal::details::unique_state{count},
          [&p1, this] { return details::distribute(p1)(*generator_); });

      std::ranges::sort(selected1);

      auto selected2 = gal::details::selecte_many(
          gal::details::unique_state{count},
          [&p2, this] { return details::distribute(p2)(*generator_); });

      std::ranges::sort(selected2);

      std::pair<Chromosome, Chromosome> children{};

      auto out1 = prepare_chromosome(children.first, 0),
           out2 = prepare_chromosome(children.second, 0);

      auto in1 = std::begin(p1), in2 = std::begin(p2);

      std::size_t current1 = 0, current2 = 0;
      for (int i = 0; i < count; ++i) {
        for (; current1 < selected1[i]; ++current1) {
          *(out1++) = *(in1++);
        }

        for (; current2 < selected2[i]; ++current2) {
          *(out2++) = *(in2++);
        }

        std::swap(out1, out2);
      }

      std::copy(in1, std::end(p1), out1);
      std::copy(in2, std::end(p2), out2);

      return children;
    }

  private:
    generator_t* generator_;
  };

} // namespace cross
} // namespace gal
