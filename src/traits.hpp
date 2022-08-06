
#pragma once

#include <concepts>
#include <type_traits>

namespace gal {
namespace traits {

  template<typename Ref, typename Decayed>
  concept forward_ref = std::same_as<Decayed, std::remove_cvref_t<Ref>>;

  template<typename Ty>
  struct is_nothrow_forward_constructible
      : std::is_nothrow_copy_constructible<std::decay_t<Ty>> {};

  template<typename Ty>
  struct is_nothrow_forward_constructible<Ty&&>
      : std::is_nothrow_move_constructible<std::decay_t<Ty>> {};

  template<typename Ty>
  constexpr inline auto is_nothrow_forward_constructible_v =
      is_nothrow_forward_constructible<Ty>::value;

  template<typename... Tys>
  struct is_nothrow_forward_constructibles
      : std::conjunction<is_nothrow_forward_constructible<Tys>...> {};

  template<typename... Tys>
  constexpr inline auto is_nothrow_forward_constructibles_v =
      is_nothrow_forward_constructibles<Tys...>::value;

  template<typename Ty>
  struct is_nothrow_forward_assignable
      : std::is_nothrow_copy_assignable<std::decay_t<Ty>> {};

  template<typename Ty>
  struct is_nothrow_forward_assignable<Ty&&>
      : std::is_nothrow_move_assignable<std::decay_t<Ty>> {};

  template<typename Ty>
  constexpr inline auto is_nothrow_forward_assignable_v =
      is_nothrow_forward_assignable<Ty>::value;

  template<typename... Tys>
  struct is_nothrow_forward_assignables
      : std::conjunction<is_nothrow_forward_assignable<Tys>...> {};

  template<typename... Tys>
  constexpr inline auto is_nothrow_forward_assignables_v =
      is_nothrow_forward_assignables<Tys...>::value;

  template<typename Ty>
  concept boolean_flag = std::derived_from<Ty, std::true_type> ||
      std::derived_from<Ty, std::false_type>;
} // namespace traits
} // namespace gal
