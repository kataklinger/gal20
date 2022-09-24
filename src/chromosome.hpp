
#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <vector>

namespace gal {

template<typename Type>
concept chromosome = std::regular<Type>;

template<typename Chromosome>
concept range_chromosome =
    chromosome<Chromosome> && std::ranges::bidirectional_range<Chromosome> &&
    std::ranges::sized_range<Chromosome>;

template<range_chromosome Chromosome>
inline auto draft(Chromosome& target, std::size_t /*unused*/) noexcept {
  return std::back_inserter(target);
}

template<typename Value, std::size_t Size>
inline auto draft(std::array<Value, Size>& target,
                  std::size_t /*unused*/) noexcept {
  return std::begin(target);
}

template<typename Value, std::size_t Size>
inline auto draft(Value (&target)[Size], std::size_t /*unused*/) noexcept {
  return std::begin(target);
}

template<typename... Tys>
inline auto draft(std::vector<Tys...>& target, std::size_t size) {
  target.reserve(size);
  return std::back_inserter(target);
}

} // namespace gal
