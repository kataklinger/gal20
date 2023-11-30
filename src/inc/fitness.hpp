
#pragma once

#include <compare>
#include <concepts>
#include <random>
#include <type_traits>

#include "utility.hpp"

namespace gal {

template<typename Type>
concept fitness =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

template<typename Operation, typename Fitness>
concept comparator =
    fitness<Fitness> &&
    std::is_invocable_r_v<
        bool,
        Operation,
        std::add_lvalue_reference_t<std::add_const_t<Fitness>>,
        std::add_lvalue_reference_t<std::add_const_t<Fitness>>>;

struct disabled_comparator {
  template<typename Fitness>
  inline constexpr auto operator()(Fitness const& /*unused*/,
                                   Fitness const& /*unused*/) const noexcept {
    return false;
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
      std::assignable_from<Type, int>;

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

template<fitness Fitness>
struct multiobjective_value {
  using type = std::remove_reference_t<decltype(*std::ranges::begin(
      std::declval<std::add_lvalue_reference_t<Fitness>>()))>;
};

template<fitness Fitness>
using multiobjective_value_t = typename multiobjective_value<Fitness>::type;

template<typename Fitness>
concept multiobjective_fitness =
    std::ranges::sized_range<Fitness> &&
    std::ranges::random_access_range<Fitness> &&
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
  inline std::weak_ordering operator()(Fitness const& left,
                                       Fitness const& right) const {
    bool res_l{}, res_r{};

    auto end = std::ranges::end(left);
    for (auto itl = std::ranges::begin(left), itr = std::ranges::begin(right);
         itl != end && (!res_l || !res_r);
         ++itl, ++itr) {
      res_l = res_l || compare_(*itl, *itr);
      res_r = res_r || compare_(*itr, *itl);
    }

    if (res_l) {
      if (!res_r) {
        return std::weak_ordering::less;
      }
    }
    else if (res_r) {
      return std::weak_ordering::greater;
    }

    return std::weak_ordering::equivalent;
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
  inline evaluation() noexcept {
  }

  inline explicit evaluation(raw_t const& raw) noexcept(
      std::is_nothrow_copy_constructible_v<raw_t>)
      : raw_{raw} {
  }

  inline explicit evaluation(raw_t&& raw) noexcept
      : raw_{std::move(raw)} {
  }

  inline explicit evaluation(raw_t const& raw, scaled_t const& scaled) noexcept(
      std::is_nothrow_copy_constructible_v<raw_t> &&
      std::is_nothrow_copy_constructible_v<scaled_t>)
      : raw_{raw}
      , scaled_{scaled} {
  }

  inline explicit evaluation(raw_t&& raw, scaled_t&& scaled) noexcept
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
