#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace gal::literals {

namespace details {

  template<std::size_t Size>
  struct rep;

  template<>
  struct rep<4> {
    using type = std::uint32_t;
  };

  template<>
  struct rep<8> {
    using type = std::uint64_t;
  };

  template<typename Ty>
  using rep_t = typename rep<sizeof(Ty)>::type;

} // namespace details

template<std::floating_point Value>
struct fp_const {
  using value_t = Value;

  fp_const() = default;

  inline constexpr fp_const(value_t value) noexcept
      : rep_{std::bit_cast<details::rep_t<value_t>>(value)} {
  }

  inline constexpr value_t value() const noexcept {
    return std::bit_cast<value_t>(rep_);
  }

  inline constexpr operator value_t() const noexcept {
    return value();
  }

  details::rep_t<value_t> rep_{};
};

inline constexpr fp_const<double> operator""_dc(long double value) noexcept {
  return fp_const{static_cast<double>(value)};
}

inline constexpr fp_const<float> operator""_fc(long double value) noexcept {
  return fp_const{static_cast<float>(value)};
}

template<typename Ty>
struct underlying_const_type_impl {
  using type = Ty;
};

template<typename Ty>
struct underlying_const_type_impl<fp_const<Ty>> {
  using type = typename fp_const<Ty>::value_t;
};

template<typename Ty>
using underlying_const_type = typename underlying_const_type_impl<Ty>::type;

} // namespace gal::literals
