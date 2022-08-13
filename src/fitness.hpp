
#pragma once

#include <concepts>
#include <type_traits>

namespace gal {

template<typename Type>
concept fitness =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

template<typename Type>
concept ordered_fitness = fitness<Type> && std::totally_ordered<Type>;

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

struct raw_fitness_tag {};
struct scaled_fitness_tag {};

} // namespace gal
