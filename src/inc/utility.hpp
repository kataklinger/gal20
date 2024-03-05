
#pragma once

#include <concepts>
#include <type_traits>

#include "literals.hpp"

namespace gal::util {

template<typename Ref, typename Decayed>
concept forward_ref = std::same_as<Decayed, std::remove_cvref_t<Ref>>;

template<typename Ty>
struct is_nothrow_forward_constructible
    : std::is_nothrow_copy_constructible<std::decay_t<Ty>> {};

template<typename Ty>
struct is_nothrow_forward_constructible<Ty&&>
    : std::is_nothrow_move_constructible<std::decay_t<Ty>> {};

template<typename Ty>
inline constexpr auto is_nothrow_forward_constructible_v =
    is_nothrow_forward_constructible<Ty>::value;

template<typename... Tys>
struct is_nothrow_forward_constructibles
    : std::conjunction<is_nothrow_forward_constructible<Tys>...> {};

template<typename... Tys>
inline constexpr auto is_nothrow_forward_constructibles_v =
    is_nothrow_forward_constructibles<Tys...>::value;

template<typename Ty>
struct is_nothrow_forward_assignable
    : std::is_nothrow_copy_assignable<std::decay_t<Ty>> {};

template<typename Ty>
struct is_nothrow_forward_assignable<Ty&&>
    : std::is_nothrow_move_assignable<std::decay_t<Ty>> {};

template<typename Ty>
inline constexpr auto is_nothrow_forward_assignable_v =
    is_nothrow_forward_assignable<Ty>::value;

template<typename... Tys>
struct is_nothrow_forward_assignables
    : std::conjunction<is_nothrow_forward_assignable<Tys>...> {};

template<typename... Tys>
inline constexpr auto is_nothrow_forward_assignables_v =
    is_nothrow_forward_assignables<Tys...>::value;

template<typename Ty>
concept boolean_flag = std::derived_from<Ty, std::true_type> ||
                       std::derived_from<Ty, std::false_type>;

template<typename From, typename To>
concept decays_to = std::same_as<std::decay_t<From>, To>;

template<typename Value>
concept runtime_arithmetic = std::integral<Value> || std::floating_point<Value>;

template<typename Value>
concept compile_arithmetic =
    std::integral<Value> ||
    std::same_as<std::remove_cv_t<Value>, literals::fp_const<float>> ||
    std::same_as<std::remove_cv_t<Value>, literals::fp_const<double>>;

template<typename Ty, boolean_flag Condition>
inline decltype(auto) move_if(Ty&& value, Condition /*unused*/) noexcept {
  if constexpr (Condition::value) {
    return std::move(value);
  }
  else {
    return std::forward<Ty>(value);
  }
}

} // namespace gal::util
