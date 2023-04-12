
#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <vector>

#include "utility.hpp"

namespace gal {

template<typename Type>
concept chromosome = std::regular<Type>;

template<typename Chromosome>
concept range_chromosome =
    chromosome<Chromosome> && std::ranges::bidirectional_range<Chromosome> &&
    std::ranges::sized_range<Chromosome>;

template<typename Operation, typename Chromosome>
concept matcher =
    chromosome<Chromosome> &&
    std::is_invocable_v<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>> &&
    util::arithmetic<std::invoke_result_t<
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>,
        std::add_lvalue_reference_t<std::add_const_t<Chromosome>>>>;

template<range_chromosome Chromosome>
inline auto draft(Chromosome& target, std::size_t /*unused*/) noexcept {
  return std::back_inserter(target);
}

template<typename Value, std::size_t Size>
inline auto draft(std::array<Value, Size>& target,
                  std::size_t /*unused*/) noexcept {
  return std::ranges::begin(target);
}

template<typename Value, std::size_t Size>
inline auto draft(Value (&target)[Size], std::size_t /*unused*/) noexcept {
  return std::ranges::begin(target);
}

template<typename... Tys>
inline auto draft(std::vector<Tys...>& target, std::size_t size) {
  target.reserve(size);
  return std::back_inserter(target);
}

} // namespace gal
