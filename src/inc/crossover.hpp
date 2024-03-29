
#pragma once

#include "sampling.hpp"

namespace gal::cross {

namespace details {

  template<range_chromosome Chromosome>
  inline auto shorter(Chromosome const& parent1,
                      Chromosome const& parent2) noexcept {
    return std::min(std::ranges::size(parent1), std::ranges::size(parent2));
  }

  template<range_chromosome Chromosome>
  inline void splice(Chromosome& dest,
                     Chromosome const& left,
                     std::size_t point_left,
                     Chromosome const& right,
                     std::size_t point_right) {
    auto size = point_left + std::ranges::size(right) - point_right;

    std::ranges::copy(std::next(std::ranges::begin(right), point_right),
                      std::ranges::end(right),
                      std::ranges::copy_n(std::ranges::begin(left),
                                          point_left,
                                          draft(dest, size))
                          .out);
  }

} // namespace details

template<typename Generator>
class symmetric_singlepoint {
public:
  using generator_t = Generator;

public:
  inline explicit symmetric_singlepoint(generator_t const& generator)
      : generator_{generator} {
  }

  template<range_chromosome Chromosome>
  inline auto operator()(Chromosome const& p1, Chromosome const& p2) const {
    auto pt = std::invoke(std::invoke(generator_, 1, details::shorter(p1, p2)));

    std::pair<Chromosome, Chromosome> children{};

    details::splice(children.first, p1, pt, p2, pt);
    details::splice(children.second, p2, pt, p1, pt);

    return children;
  }

private:
  generator_t generator_;
};

template<typename Generator>
class asymmetric_singlepoint {
public:
  using generator_t = Generator;

public:
  inline explicit asymmetric_singlepoint(generator_t const& generator)
      : generator_{generator} {
  }

  template<range_chromosome Chromosome>
  inline auto operator()(Chromosome const& p1, Chromosome const& p2) const {
    auto pt1 = std::invoke(std::invoke(generator_, 1, p1));
    auto pt2 = std::invoke(std::invoke(generator_, 1, p2));

    std::pair<Chromosome, Chromosome> children{};

    details::splice(children.first, p1, pt1, p2, pt2);
    details::splice(children.second, p2, pt2, p1, pt1);

    return children;
  }

private:
  generator_t generator_;
};

template<typename Generator, std::size_t Points>
  requires(Points > 1)
class symmetric_multipoint {
public:
  using generator_t = Generator;

  inline static constexpr auto points = Points;

public:
  inline explicit symmetric_multipoint(generator_t const& generator,
                                       countable_t<Points> /*unused*/)
      : generator_{generator} {
  }

  template<range_chromosome Chromosome>
  auto operator()(Chromosome const& p1, Chromosome const& p2) const {
    auto size1 = std::ranges::size(p1), size2 = std::ranges::size(p2);
    auto count = std::min({points, size1 - 1, size2 - 1});

    if (count == 0) {
      return std::pair<Chromosome, Chromosome>{p1, p2};
    }

    auto selected = sample_many(
        unique_sample{count},
        [rnd = std::invoke(generator_, 1, details::shorter(p1, p2))]() mutable {
          return std::invoke(rnd);
        });

    std::ranges::sort(selected);

    if (selected.size() % 2 == 1) {
      std::ranges::swap(size1, size2);
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
    std::pair in{std::ranges::begin(p1), std::ranges::begin(p2)};
    for (auto point : selected) {
      std::pair next{std::ranges::begin(p1) + point,
                     std::ranges::begin(p2) + point};

      out1 = std::ranges::copy(in.first, next.first, out1).out;
      out2 = std::ranges::copy(in.second, next.second, out2).out;

      in = next;
      std::ranges::swap(out1, out2);
    }

    std::ranges::copy(in.first, std::ranges::end(p1), out1);
    std::ranges::copy(in.second, std::ranges::end(p2), out2);
  }

  template<std::ranges::forward_range Chromosome, typename It>
  auto splice(Chromosome const& p1,
              Chromosome const& p2,
              std::vector<std::size_t> const& selected,
              It out1,
              It out2) const {
    auto in1 = std::ranges::begin(p1), in2 = std::ranges::begin(p2);

    std::size_t current = 0;
    for (auto point : selected) {
      for (; current < point; ++current) {
        *(out1++) = *(in1++);
        *(out2++) = *(in2++);
      }

      std::ranges::swap(out1, out2);
    }

    std::ranges::copy(in1, std::ranges::end(p1), out1);
    std::ranges::copy(in2, std::ranges::end(p2), out2);
  }

private:
  generator_t generator_;
};

template<typename Generator, std::size_t Points>
  requires(Points > 1)
class asymmetric_multipoint {
public:
  using generator_t = Generator;

  inline static constexpr auto points = Points;

public:
  inline explicit asymmetric_multipoint(generator_t const& generator,
                                        countable_t<Points> /*unused*/)
      : generator_{generator} {
  }

  template<range_chromosome Chromosome>
  auto operator()(Chromosome const& p1, Chromosome const& p2) const {
    auto size1 = std::ranges::size(p1), size2 = std::ranges::size(p2);
    auto count = std::min({points, size1 - 1, size2 - 1});

    if (count == 0) {
      return std::pair<Chromosome, Chromosome>{p1, p2};
    }

    auto selected1 = sample_many(
        unique_sample{count}, [rnd = std::invoke(generator_, 1, p1)]() mutable {
          return std::invoke(rnd);
        });

    std::ranges::sort(selected1);

    auto selected2 = sample_many(
        unique_sample{count}, [rnd = std::invoke(generator_, 1, p2)]() mutable {
          return std::invoke(rnd);
        });

    std::ranges::sort(selected2);

    std::pair<Chromosome, Chromosome> children{};

    auto out1 = draft(children.first, 0), out2 = draft(children.second, 0);

    auto in1 = std::ranges::begin(p1), in2 = std::ranges::begin(p2);

    std::size_t current1 = 0, current2 = 0;
    for (std::size_t i = 0; i < count; ++i) {
      for (; current1 < selected1[i]; ++current1) {
        *(out1++) = *(in1++);
      }

      for (; current2 < selected2[i]; ++current2) {
        *(out2++) = *(in2++);
      }

      std::ranges::swap(out1, out2);
    }

    std::ranges::copy(in1, std::ranges::end(p1), out1);
    std::ranges::copy(in2, std::ranges::end(p2), out2);

    return children;
  }

private:
  generator_t generator_;
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
    namespace rg = std::ranges;

    auto size1 = rg::size(p1), size2 = rg::size(p2);

    std::pair<Chromosome, Chromosome> children{};

    auto out1 = draft(children.first, size1),
         out2 = draft(children.second, size2);

    auto in1 = rg::begin(p1), in2 = rg::begin(p2);

    auto [short_it, short_end, long_it, long_end, long_out] =
        size1 < size2 ? std::tuple{in1, rg::end(p1), in2, rg::end(p2), out2}
                      : std::tuple{in2, rg::end(p2), in1, rg::end(p1), out1};

    for (; short_it != short_end; ++in1, ++in2, ++short_it, ++long_it) {
      auto [v1, v2] = blender_(*in1, *in2);
      *(out1++) = std::move(v1);
      *(out2++) = std::move(v2);
    }

    std::ranges::copy(long_it, long_end, long_out);

    return children;
  }

private:
  blender_r blender_;
};

} // namespace gal::cross
