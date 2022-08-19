
#pragma once

#include "chromosome.hpp"
#include "fitness.hpp"

#include "traits.hpp"

#include <ranges>
#include <optional>

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
  template<traits::forward_ref<chromosome_t> C,
           traits::forward_ref<evaluation_t> E,
           std::default_initializable T = tags_t>
  inline explicit individual(C&& chromosome, E&& evaluation) noexcept(
      traits::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                  decltype(evaluation)>&&
          std::is_nothrow_default_constructible_v<tags_t>)
      : chromosome_{std::forward<C>(chromosome)}
      , evaluation_{std::forward<E>(evaluation)}
      , tags_{} {
  }

  template<traits::forward_ref<chromosome_t> C,
           traits::forward_ref<evaluation_t> E,
           traits::forward_ref<tags_t> T>
  inline individual(C&& chromosome, E&& evaluation, T&& tags) noexcept(
      traits::is_nothrow_forward_constructibles_v<decltype(chromosome),
                                                  decltype(evaluation),
                                                  decltype(tags)>)
      : chromosome_{std::forward<C>(chromosome)}
      , evaluation_{std::forward<E>(evaluation)}
      , tags_{std::forward<T>(tags)} {
  }

  inline void swap(individual& other) noexcept(
      std::is_nothrow_swappable_v<individual>) {
    using std::swap;

    swap(chromosome_, other.chromosome_);
    std::swap(evaluation_, other.evaluation_);
    swap(tags_, other.tags_);
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

using rank_t = std::optional<std::size_t>;

template<typename Range, typename Selected>
concept selection_range = std::ranges::range<Range> && requires(Range r) {
  requires std::same_as<std::remove_cvref_t<decltype(*std::ranges::begin(r))>,
                        Selected>;
};

template<typename Range, typename Replaced, typename Replacement>
concept replacement_range = std::ranges::range<Range> && requires(Range r) {
  requires std::same_as<
      std::remove_cvref_t<decltype(get_parent(*std::ranges::begin(r)))>,
      Replaced>;
  requires std::same_as<
      std::remove_cvref_t<decltype(get_child(*std::ranges::begin(r)))>,
      Replacement>;
};

} // namespace gal