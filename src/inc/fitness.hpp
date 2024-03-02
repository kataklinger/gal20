
#pragma once

#include <compare>
#include <concepts>
#include <functional>
#include <random>
#include <type_traits>

#include "utility.hpp"

namespace gal {

template<typename Type>
concept fitness =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

template<typename Operation, typename Fitness>
using comparator_ordering_t = std::invoke_result_t<
    Operation,
    std::add_lvalue_reference_t<std::add_const_t<Fitness>>,
    std::add_lvalue_reference_t<std::add_const_t<Fitness>>>;

template<typename Operation, typename Fitness>
concept comparator =
    fitness<Fitness> &&
    std::invocable<Operation,
                   std::add_lvalue_reference_t<std::add_const_t<Fitness>>,
                   std::add_lvalue_reference_t<std::add_const_t<Fitness>>> &&
    std::convertible_to<comparator_ordering_t<Operation, Fitness>,
                        std::partial_ordering>;

struct disabled_comparator {
  template<typename Fitness>
  inline constexpr auto operator()(Fitness const& /*unused*/,
                                   Fitness const& /*unused*/) const noexcept {
    return std::partial_ordering::unordered;
  }
};

template<typename Comparator, typename Fitness>
struct comparator_traits {
  using fitness_t = Fitness;
  using comparator_t = Comparator;

  using ordering_t = comparator_ordering_t<Comparator, Fitness>;

  inline static constexpr auto is_enabled =
      !std::same_as<comparator_t, disabled_comparator>;
};

namespace details {

  struct partial_order_less {
    inline static constexpr auto value = std::partial_ordering::less;
  };

  struct partial_order_greater {
    inline static constexpr auto value = std::partial_ordering::greater;
  };

  template<typename Comparator, typename Bound>
  struct fitness_cmp_impl {
    Comparator cmp_;

    template<typename Ty>
      requires(std::convertible_to<comparator_ordering_t<Comparator, Ty>,
                                   std::partial_ordering>)
    inline auto operator()(Ty const& left, Ty const& right) const noexcept(
        noexcept(std::invoke(std::declval<Comparator>(), left, right))) {
      return std::invoke(cmp_, left, right) == Bound::value;
    }
  };

} // namespace details

template<typename Comparator>
struct fitness_worse
    : details::fitness_cmp_impl<Comparator, details::partial_order_less> {};

template<typename Comparator>
fitness_worse(Comparator&&) -> fitness_worse<Comparator>;

template<typename Comparator>
struct fitness_better
    : details::fitness_cmp_impl<Comparator, details::partial_order_greater> {};

template<typename Comparator>
fitness_better(Comparator&&) -> fitness_better<Comparator>;

enum class nan_three_way { smallest, greatest };

namespace details {

  struct nan_order_less {
    inline static constexpr auto order = std::weak_ordering::less;
  };

  struct nan_order_greater {
    inline static constexpr auto order = std::weak_ordering::greater;
  };

} // namespace details

template<nan_three_way Nan = nan_three_way::smallest>
struct floatingpoint_three_way {
  using order = std::weak_ordering;

  inline static constexpr auto order_left_nan =
      Nan == nan_three_way::smallest ? order::less : order::greater;

  inline static constexpr auto order_right_nan =
      Nan == nan_three_way::smallest ? order::greater : order::less;

  template<std::floating_point Ty, std::floating_point Tx>
  inline auto operator()(Ty left, Tx right) const {
    auto left_nan = std::isnan(left), right_nan = std::isnan(right);

    return left_nan       ? (right_nan ? order::equivalent : order_left_nan)
           : right_nan    ? order_right_nan
           : right < left ? order::greater
           : left < right ? order::less
                          : order::equivalent;
  }
};

using max_floatingpoint_three_way =
    floatingpoint_three_way<nan_three_way::smallest>;

using min_floatingpoint_three_way =
    floatingpoint_three_way<nan_three_way::greatest>;

namespace details {

  template<typename Ty, typename LeftNan, typename RightNan>
  struct is_proper_nan_ordered : std::true_type {};

  template<nan_three_way Nan, typename LeftNan, typename RightNan>
  struct is_proper_nan_ordered<floatingpoint_three_way<Nan>, LeftNan, RightNan>
      : std::conjunction<
            std::bool_constant<floatingpoint_three_way<Nan>::order_left_nan ==
                               LeftNan::order>,
            std::bool_constant<floatingpoint_three_way<Nan>::order_right_nan ==
                               RightNan::order>> {};

} // namespace details

template<typename Ty>
concept maximizable =
    details::is_proper_nan_ordered<Ty,
                                   details::nan_order_less,
                                   details::nan_order_greater>::value;

template<typename Ty>
concept minimizable =
    details::is_proper_nan_ordered<Ty,
                                   details::nan_order_greater,
                                   details::nan_order_less>::value;

template<maximizable Comparator>
struct maximize {
  Comparator cmp_;

  template<typename Ty, typename Tx>
  inline auto operator()(Ty const& left, Tx const& right) const
      noexcept(noexcept(std::declval<Comparator const&>()(left, right))) {
    return cmp_(left, right);
  }
};

template<minimizable Comparator>
struct minimize {
  Comparator cmp_;

  template<typename Ty, typename Tx>
  inline auto operator()(Ty const& left, Tx const& right) const
      noexcept(noexcept(std::declval<Comparator const&>()(left, right))) {
    return cmp_(right, left);
  }
};

namespace details {

  template<typename Type>
  concept additive = requires(Type a) {
    { a + a } -> std::same_as<Type>;
    { a - a } -> std::same_as<Type>;
  };

  template<typename Type>
  concept averageable = requires(Type a) {
    { a / std::size_t(1) } -> std::same_as<Type>;
  };

  template<typename Type>
  concept divisable = requires(Type a) {
    { a / a } -> std::same_as<Type>;
  };

  template<typename Type>
  concept spatial_value = additive<Type> && std::convertible_to<Type, double>;

  template<typename Type>
  concept grid_point =
      additive<Type> && divisable<Type> && std::constructible_from<Type, int> &&
      std::assignable_from<std::add_lvalue_reference_t<Type>, int>;

} // namespace details

template<typename Type>
concept averageable_fitness =
    fitness<Type> && details::additive<Type> && details::averageable<Type>;

template<typename Fitness>
concept integer_fitness =
    averageable_fitness<Fitness> && std::integral<Fitness>;

template<typename Fitness>
concept real_fitness =
    averageable_fitness<Fitness> && std::floating_point<Fitness>;

namespace details {

  template<typename Fitness>
  concept multiobjective_fitness_impl =
      fitness<Fitness> && std::ranges::sized_range<Fitness> &&
      std::ranges::random_access_range<Fitness>;

}

template<details::multiobjective_fitness_impl Fitness>
struct multiobjective_value {
  using type = std::ranges::range_value_t<Fitness>;
};

template<details::multiobjective_fitness_impl Fitness>
using multiobjective_value_t = typename multiobjective_value<Fitness>::type;

template<typename Fitness>
concept multiobjective_fitness =
    details::multiobjective_fitness_impl<Fitness> &&
    std::totally_ordered<multiobjective_value_t<Fitness>>;

template<typename Fitness>
concept spatial_fitness =
    multiobjective_fitness<Fitness> &&
    details::spatial_value<multiobjective_value_t<Fitness>>;

template<typename Fitness>
concept grid_fitness = multiobjective_fitness<Fitness> &&
                       details::grid_point<multiobjective_value_t<Fitness>>;

template<spatial_fitness Fitness>
inline auto euclidean_distance(Fitness const& left,
                               Fitness const& right) noexcept {
  double result = 0.;

  auto ir = std::ranges::begin(right);
  for (auto&& vl : left) {
    auto d = static_cast<double>(vl - *ir);
    result += d * d;

    ++ir;
  }

  return std::sqrt(result);
}

template<typename Totalizator>
concept fitness_totalizator =
    std::semiregular<Totalizator> && requires(Totalizator t) {
      requires averageable_fitness<typename Totalizator::value_t>;

      {
        t.add(std::declval<typename Totalizator::value_t>())
      } -> std::convertible_to<Totalizator>;

      { t.sum() } -> std::convertible_to<typename Totalizator::value_t>;
    };

template<averageable_fitness Value>
class integer_fitness_totalizator {
public:
  using value_t = Value;

public:
  inline integer_fitness_totalizator() = default;

  inline auto add(value_t const& value) const noexcept {
    return integer_fitness_totalizator{sum_ + value};
  }

  inline value_t const& sum() const noexcept {
    return sum_;
  }

private:
  inline integer_fitness_totalizator(value_t const& sum)
      : sum_{sum} {
  }

private:
  value_t sum_{};
};

template<averageable_fitness Value>
class real_fitness_totalizator {
public:
  using value_t = Value;

public:
  inline real_fitness_totalizator() = default;

  inline auto add(value_t const& value) const noexcept {
    value_t y = value - correction_;
    value_t t = sum_ + y;
    return real_fitness_totalizator{t, (t - sum_) - y};
  }

  inline value_t const& sum() const noexcept {
    return sum_;
  }

private:
  inline real_fitness_totalizator(value_t const& sum, value_t const& correction)
      : sum_{sum}
      , correction_{correction} {
  }

private:
  value_t sum_{};
  value_t correction_{};
};

template<fitness Fitness>
struct fitness_traits;

template<integer_fitness Fitness>
struct fitness_traits<Fitness> {
  using totalizator_t = integer_fitness_totalizator<Fitness>;
  using random_distribution_t = std::uniform_int_distribution<Fitness>;
};

template<real_fitness Fitness>
struct fitness_traits<Fitness> {
  using totalizator_t = real_fitness_totalizator<Fitness>;
  using random_distribution_t = std::uniform_real_distribution<Fitness>;
};

struct empty_fitness {
  inline constexpr auto
      operator<=>(empty_fitness const& /*unused*/) const = default;
};

struct empty_tags {};

template<typename Fitness>
struct is_empty_fitness : std::is_same<Fitness, empty_fitness> {};

template<typename Fitness>
inline constexpr auto is_empty_fitness_v = is_empty_fitness<Fitness>::value;

template<typename ComponentCompare>
class dominate {
public:
  using component_compare_t = ComponentCompare;

public:
  inline dominate() noexcept(
      std::is_nothrow_default_constructible_v<component_compare_t>) {
  }

  inline explicit dominate(component_compare_t const& compare) noexcept(
      std::is_nothrow_copy_constructible_v<component_compare_t>)
      : compare_{compare} {
  }

  template<multiobjective_fitness Fitness>
  inline std::partial_ordering operator()(Fitness const& left,
                                          Fitness const& right) const {
    bool res_l{}, res_r{};

    auto itl = std::ranges::begin(left), end = std::ranges::end(left);
    for (auto itr = std::ranges::begin(right); itl != end && (!res_l || !res_r);
         ++itl, ++itr) {
      res_l = res_l || std::invoke(compare_, *itl, *itr);
      res_r = res_r || std::invoke(compare_, *itr, *itl);
    }

    if (res_l) {
      if (!res_r) {
        return std::partial_ordering::less;
      }
    }
    else if (res_r) {
      return std::partial_ordering::greater;
    }

    return itl == end ? std::partial_ordering::equivalent
                      : std::partial_ordering::unordered;
  }

private:
  [[no_unique_address]] component_compare_t compare_{};
};

struct raw_fitness_tag {};
struct scaled_fitness_tag {};

template<fitness Raw, fitness Scaled>
class evaluation {
public:
  using raw_t = Raw;
  using scaled_t = Scaled;

public:
  inline constexpr evaluation() noexcept {
  }

  inline constexpr explicit evaluation(raw_t const& raw) noexcept(
      std::is_nothrow_copy_constructible_v<raw_t>)
      : raw_{raw} {
  }

  inline constexpr explicit evaluation(raw_t&& raw) noexcept
      : raw_{std::move(raw)} {
  }

  inline constexpr explicit evaluation(
      raw_t const& raw,
      scaled_t const&
          scaled) noexcept(std::is_nothrow_copy_constructible_v<raw_t> &&
                           std::is_nothrow_copy_constructible_v<scaled_t>)
      : raw_{raw}
      , scaled_{scaled} {
  }

  inline constexpr explicit evaluation(raw_t&& raw, scaled_t&& scaled) noexcept
      : raw_{std::move(raw)}
      , scaled_{std::move(scaled)} {
  }

  inline void swap(evaluation& other) noexcept(
      std::is_nothrow_swappable_v<evaluation>) {
    std::ranges::swap(raw_, other.raw_);
    std::ranges::swap(scaled_, other.scaled_);
  }

  template<util::forward_ref<raw_t> F>
  inline void set_raw(F&& fitness) {
    raw_ = std::forward<F>(fitness);
  }

  template<util::forward_ref<scaled_t> F>
  inline void set_scaled(F&& fitness) {
    scaled_ = std::forward<F>(fitness);
  }

  template<util::forward_ref<raw_t> F>
  inline void set(raw_fitness_tag /*unused*/, F&& fitness) const noexcept {
    set_raw(std::forward<F>(fitness));
  }

  template<util::forward_ref<scaled_t> F>
  inline void set(scaled_fitness_tag /*unused*/, F&& fitness) const noexcept {
    set_scaled(std::forward<F>(fitness));
  }

  inline auto const& raw() const noexcept {
    return raw_;
  }

  inline auto const& scaled() const noexcept {
    return scaled_;
  }

  inline auto const& get(raw_fitness_tag /*unused*/) const noexcept {
    return raw();
  }

  inline auto const& get(scaled_fitness_tag /*unused*/) const noexcept {
    return scaled();
  }

private:
  raw_t raw_{};
  [[no_unique_address]] scaled_t scaled_{};
};

} // namespace gal
