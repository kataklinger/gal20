
#pragma once

#include <concepts>
#include <random>
#include <type_traits>

#include "traits.hpp"

namespace gal {

template<typename Type>
concept fitness =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

template<typename Operation, typename Fitness>
concept comparator = fitness<Fitness> && std::is_invocable_r_v<
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

template<typename Type>
concept arithmetic_fitness = fitness<Type> && requires(Type a) {
  { a + a } -> std::same_as<Type>;
  { a - a } -> std::same_as<Type>;
  { a / std::size_t(1) } -> std::same_as<Type>;
};

template<typename Fitness>
concept integer_fitness = arithmetic_fitness<Fitness> && std::integral<Fitness>;

template<typename Fitness>
concept real_fitness =
    arithmetic_fitness<Fitness> && std::floating_point<Fitness>;

template<typename Totalizator>
concept fitness_totalizator = requires(Totalizator t) {
  std::semiregular<Totalizator>;
  arithmetic_fitness<typename Totalizator::value_t>;

  {
    t.add(std::declval<typename Totalizator::value_t>())
    } -> std::convertible_to<Totalizator>;

  { t.sum() } -> std::convertible_to<typename Totalizator::value_t>;
};

template<arithmetic_fitness Value>
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

template<arithmetic_fitness Value>
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
      std::is_nothrow_copy_constructible_v<raw_t>&&
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
    using std::swap;

    swap(raw_, other.raw_);
    swap(scaled_, other.scaled_);
  }

  template<traits::forward_ref<raw_t> F>
  inline void set_raw(F&& fitness) {
    raw_ = std::forward<F>(fitness);
  }

  template<traits::forward_ref<scaled_t> F>
  inline void set_scaled(F&& fitness) {
    scaled_ = std::forward<F>(fitness);
  }

  template<traits::forward_ref<raw_t> F>
  inline void set(raw_fitness_tag /*unused*/, F&& fitness) const noexcept {
    set_raw(std::forward<F>(fitness));
  }

  template<traits::forward_ref<scaled_t> F>
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
