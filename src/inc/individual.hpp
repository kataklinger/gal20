
#pragma once

#include "chromosome.hpp"
#include "fitness.hpp"
#include "utility.hpp"

#include <cassert>
#include <optional>
#include <ranges>

namespace gal {

template<chromosome Chromosome, fitness Raw, fitness Scaled, typename Tags>
class individual {
public:
  using chromosome_t = Chromosome;
  using raw_fitness_t = Raw;
  using scaled_fitness_t = Scaled;
  using evaluation_t = evaluation<raw_fitness_t, scaled_fitness_t>;
  using tags_t = Tags;

public:
  template<util::forward_ref<chromosome_t> C, util::forward_ref<evaluation_t> E>
  inline individual(C&& chromosome, E&& evaluation) noexcept(
      util::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                decltype(evaluation)> &&
      std::is_nothrow_default_constructible_v<tags_t>)
      : chromosome_{std::forward<C>(chromosome)}
      , evaluation_{std::forward<E>(evaluation)}
      , tags_{} {
  }

  template<util::forward_ref<chromosome_t> C,
           util::forward_ref<evaluation_t> E,
           util::forward_ref<tags_t> T>
  inline individual(C&& chromosome, E&& evaluation, T&& tags) noexcept(
      util::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                decltype(evaluation),
                                                decltype(tags)>)
      : chromosome_{std::forward<C>(chromosome)}
      , evaluation_{std::forward<E>(evaluation)}
      , tags_{std::forward<T>(tags)} {
  }

  template<util::forward_ref<chromosome_t> C,
           util::forward_ref<raw_fitness_t> F>
  inline individual(C&& chromosome, F&& fitness) noexcept(
      util::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                decltype(fitness)> &&
      std::is_nothrow_default_constructible_v<tags_t>)
      : individual{std::forward<C>(chromosome),
                   evaluation_t{std::forward<F>(fitness)}} {
  }

  template<util::forward_ref<chromosome_t> C,
           util::forward_ref<raw_fitness_t> F,
           util::forward_ref<tags_t> T>
  inline individual(C&& chromosome, F&& fitness, T&& tags) noexcept(
      util::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                decltype(fitness),
                                                decltype(tags)>)
      : individual{std::forward<C>(chromosome),
                   evaluation_t{std::forward<F>(fitness)},
                   std::forward<T>(tags)} {
  }

  inline void swap(individual& other) noexcept(
      std::is_nothrow_swappable_v<individual>) {
    std::ranges::swap(chromosome_, other.chromosome_);
    std::ranges::swap(evaluation_, other.evaluation_);
    std::ranges::swap(tags_, other.tags_);
  }

  inline auto const& chromosome() const noexcept {
    return chromosome_;
  }

  inline auto const& evaluation() const noexcept {
    return evaluation_;
  }

  inline auto& evaluation() noexcept {
    return evaluation_;
  }

  inline auto& tags() noexcept {
    return tags_;
  }

  inline auto const& tags() const noexcept {
    return tags_;
  }

private:
  chromosome_t chromosome_;
  evaluation_t evaluation_;
  [[no_unique_address]] tags_t tags_;
};

template<typename Tag, typename... Tys>
inline auto& get_tag(individual<Tys...>& ind) noexcept {
  if constexpr (std::is_same_v<Tag, typename individual<Tys...>::tags_t>) {
    return ind.tags();
  }
  else {
    return std::get<Tag>(ind.tags());
  }
}

template<typename Tag, typename... Tys>
inline auto const& get_tag(individual<Tys...> const& ind) noexcept {
  if constexpr (std::is_same_v<Tag, typename individual<Tys...>::tags_t>) {
    return ind.tags();
  }
  else {
    return std::get<Tag>(ind.tags());
  }
}

using ordinal_t = std::optional<std::size_t>;

template<typename Range, typename Selected>
concept selection_range =
    std::ranges::random_access_range<Range> && requires(Range r) {
      { *std::ranges::begin(r) } -> util::decays_to<Selected>;
    };

template<typename Range, typename Replaced, typename Replacement>
concept replacement_range =
    std::ranges::random_access_range<Range> && requires(Range r) {
      { get_parent(*std::ranges::begin(r)) } -> util::decays_to<Replaced>;
      { get_child(*std::ranges::begin(r)) } -> util::decays_to<Replacement>;
    };

namespace details {

  template<typename Tuple, typename Tag>
  struct has_tag_impl : std::false_type {};

  template<typename Tag>
  struct has_tag_impl<Tag, Tag> : std::true_type {};

  template<typename Tag, typename... Tys>
  struct has_tag_impl<std::tuple<Tys...>, Tag>
      : std::disjunction<std::is_same<Tag, Tys>...> {};

} // namespace details

template<typename Individual, typename... Tags>
struct is_individual_tagged_with
    : std::conjunction<
          details::has_tag_impl<typename Individual::tags_t, Tags>...> {};

template<typename Individual, typename... Tags>
inline constexpr auto is_individual_tagged_with_v =
    is_individual_tagged_with<Individual, Tags...>::value;

template<typename Individual, typename... Tags>
concept individual_tagged_with =
    is_individual_tagged_with_v<Individual, Tags...>;

template<typename Value>
concept adapted_value = std::semiregular<Value>;

template<typename Tag, adapted_value Value>
class tag_adapted_value {
public:
  using tag_t = Tag;
  using value_t = Value;

public:
  inline constexpr tag_adapted_value() noexcept(
      std::is_nothrow_default_constructible_v<value_t>) {
  }

  inline constexpr tag_adapted_value(value_t value) noexcept(
      std::is_nothrow_copy_constructible_v<value_t>)
      : value_{value} {
  }

  inline explicit operator value_t() const
      noexcept(std::is_nothrow_copy_constructible_v<value_t>) {
    return value_;
  }

  inline tag_adapted_value& operator=(value_t value) noexcept(
      std::is_nothrow_copy_assignable_v<value_t>) {
    value_ = value;
    return *this;
  }

  inline value_t get() const
      noexcept(std::is_nothrow_copy_constructible_v<value_t>) {
    return value_;
  }

  template<typename Rhs>
    requires(requires(Rhs const& rhs) { std::declval<value_t&>() += rhs; })
  inline tag_adapted_value& operator+=(Rhs const& rhs) noexcept(
      noexcept(std::declval<value_t&>() += rhs)) {
    value_ += rhs;
    return *this;
  }

  template<typename Rhs>
    requires(requires(Rhs const& rhs) { std::declval<value_t&>() -= rhs; })
  inline tag_adapted_value& operator-=(Rhs const& rhs) noexcept(
      noexcept(std::declval<value_t&>() -= rhs)) {
    value_ -= rhs;
    return *this;
  }

protected:
  value_t value_{};
};

template<typename Value>
concept order_adapted_value =
    adapted_value<Value> && std::regular<Value> && std::totally_ordered<Value>;

template<typename Tag, order_adapted_value Value>
class tag_order_adopted_value : public tag_adapted_value<Tag, Value> {
public:
  using tag_adapted_value<Tag, Value>::tag_adapted_value;

  inline auto operator<=>(tag_order_adopted_value const& rhs) const {
    return this->value_ <=> rhs.value_;
  }

  inline auto operator==(tag_order_adopted_value const& rhs) const {
    return this->value_ == rhs.value_;
  }
};

template<typename Tag>
struct tag_adopted_traits;

template<typename Tag, typename Value, template<typename...> class Adopted>
  requires(
      std::derived_from<Adopted<Tag, Value>, tag_adapted_value<Tag, Value>>)
struct tag_adopted_traits<Adopted<Tag, Value>> {
  using tag_t = typename tag_adapted_value<Tag, Value>::tag_t;
  using value_t = typename tag_adapted_value<Tag, Value>::value_t;
};

enum class ancestry_status : std::uint8_t { parent = 0, child = 1, none = 2 };

struct ancestry_tag {};

using ancestry_t = tag_adapted_value<ancestry_tag, ancestry_status>;

class cluster_label {
private:
  inline constexpr explicit cluster_label(std::size_t raw,
                                          int /*unused*/) noexcept
      : raw_{raw} {
  }

public:
  inline static constexpr auto unique() noexcept {
    return cluster_label{0b10, 0};
  }

  inline static constexpr auto unassigned() noexcept {
    return cluster_label{0b00, 0};
  }

  cluster_label() = default;

  inline constexpr explicit cluster_label(std::size_t index) noexcept
      : raw_{((index + 1) << 2) | 1} {
  }

  inline auto is_proper() const noexcept {
    return (raw_ & 1) == 1;
  }

  inline auto is_unique() const noexcept {
    return (raw_ & 1) == 0 && raw_ != 0;
  }

  inline auto is_unassigned() const noexcept {
    return raw_ == 0;
  }

  inline auto index() const noexcept {
    assert(is_proper());

    return (raw_ >> 2) - 1;
  }

  auto operator<=>(cluster_label const&) const = default;

private:
  std::size_t raw_{};
};

} // namespace gal
