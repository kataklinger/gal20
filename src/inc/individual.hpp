
#pragma once

#include "chromosome.hpp"
#include "fitness.hpp"
#include "utility.hpp"

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
inline auto& get_tag(individual<Tys...>& individual) noexcept {
  return std::get<Tag>(individual.tags());
}

template<typename Tag, typename... Tys>
inline auto const& get_tag(individual<Tys...> const& individual) noexcept {
  return std::get<Tag>(individual.tags());
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

template<typename Population, typename... Tags>
struct is_individual_tagged_with
    : std::conjunction<
          details::has_tag_impl<typename Population::individual_t::tags_t,
                                Tags>...> {};

template<typename Population, typename... Tags>
inline constexpr auto is_individual_tagged_with_v =
    is_individual_tagged_with<Population, Tags...>::value;

template<typename Individual, typename... Tags>
concept individual_tagged_with =
    is_individual_tagged_with_v<Individual, Tags...>;

} // namespace gal
