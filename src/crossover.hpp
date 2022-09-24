
#pragma once

#include "utility.hpp"

namespace gal {
namespace cross {

  namespace details {

    using distribution_t = std::uniform_int_distribution<std::size_t>;

    template<range_chromosome Chromosome>
    using get_iterator_type = std::remove_reference_t<decltype(std::begin(
        std::declval<std::add_lvalue_reference_t<Chromosome>>()))>;

    template<range_chromosome Chromosome>
    inline auto distribute(Chromosome const& parent) noexcept {
      return distribution_t{1, std::ranges::size(parent) - 1};
    }

    template<range_chromosome Chromosome>
    inline auto distribute(Chromosome const& parent1,
                           Chromosome const& parent2) noexcept {
      return distribution_t{
          1,
          std::min(std::ranges::size(parent1), std::ranges::size(parent2)) - 1};
    }

    template<range_chromosome Chromosome>
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
                std::copy_n(std::begin(left), point_left, draft(dest, size)));
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

    template<range_chromosome Chromosome>
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

    template<range_chromosome Chromosome>
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

    template<range_chromosome Chromosome>
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
      auto out1 = draft(children.first, size1),
           out2 = draft(children.second, size2);

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

    template<range_chromosome Chromosome>
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

      auto out1 = draft(children.first, 0), out2 = draft(children.second, 0);

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

  template<typename Blender>
  class blend {
    using blender_r = Blender;

  public:
    inline explicit blend(blender_r&& blender)
        : blender_{std::move(blender)} {
    }

    inline explicit blend(blender_r const& blender)
        : blender_{blender} {
    }

    template<range_chromosome Chromosome>
    auto operator()(Chromosome const& p1, Chromosome const& p2) const {
      auto size1 = std::ranges::size(p1), size2 = std::ranges::size(p2);

      std::pair<Chromosome, Chromosome> children{};

      auto out1 = draft(children.first, size1),
           out2 = draft(children.second, size2);

      auto in1 = std::begin(p1), in2 = std::begin(p2);

      auto [short_it, short_end, long_it, long_end, long_out] =
          size1 < size2
              ? std::tuple{in1, std::end(p1), in2, std::end(p2), out2}
              : std::tuple{in2, std::end(p2), in1, std::end(p1), out1};

      for (; short_it != short_end; ++in1, ++in2, ++short_it, ++long_it) {
        auto [v1, v2] = blender_(*in1, *in2);
        *(out1++) = std::move(v1);
        *(out2++) = std::move(v2);
      }

      for (; long_it != long_end; ++long_it) {
        *(long_out++) = *long_it;
      }

      return children;
    }

  private:
    blender_r blender_;
  };

} // namespace cross
} // namespace gal
