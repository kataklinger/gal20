#pragma once

#include <bit>
#include <cstddef>

namespace gal::literals {

struct fp_const {
  fp_const() = default;

  inline constexpr fp_const(double value) noexcept
      : rep_{std::bit_cast<std::size_t>(value)} {
  }

  inline constexpr double value() const noexcept {
    return std::bit_cast<double>(rep_);
  }

  inline constexpr operator double() const noexcept {
    return value();
  }

  std::size_t rep_{};
};

inline constexpr fp_const operator""_fc(long double value) noexcept {
  return fp_const{static_cast<double>(value)};
}

} // namespace gal::literals
