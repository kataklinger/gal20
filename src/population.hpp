
#pragma once

#include "individual.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace gal {

template<typename FitnessTag, typename Raw, typename Scaled>
concept sort_fitness_tag = ( std::same_as<FitnessTag, raw_fitness_tag> &&
                             ordered_fitness<Raw> ) ||
                           (std::same_as<FitnessTag, scaled_fitness_tag> &&
                            ordered_fitness<Scaled>);

enum class sort_by { none, raw, scaled, both };

template<typename FitnessTag>
struct sort_policy_base {
  const sort_by by;

  inline explicit sort_policy_base(bool stable_scaling, sort_by value) noexcept
      : by{stable_scaling ? sort_by::both : value} {
  }

  template<typename Collection>
  inline auto minmax(Collection const& individuals) noexcept {
    return std::ranges::minmax_element(
        individuals, std::ranges::greater{}, [](auto const& i) {
          return i.evaluation().get(FitnessTag{});
        });
  }

  template<typename Collection>
  inline void sort(Collection& individuals) {
    std::ranges::sort(individuals, std::ranges::greater{}, [](auto const& i) {
      return i.evaluation().get(FitnessTag{});
    });
  }
};

template<typename FitnessTag>
struct sort_policy {};

template<>
struct sort_policy<raw_fitness_tag> : sort_policy_base<raw_fitness_tag> {
  inline explicit sort_policy(bool stable_scaling) noexcept
      : sort_policy_base<raw_fitness_tag>{stable_scaling, sort_by::raw} {
  }
};

template<>
struct sort_policy<scaled_fitness_tag> : sort_policy_base<scaled_fitness_tag> {
  inline explicit sort_policy(bool stable_scaling) noexcept
      : sort_policy_base<scaled_fitness_tag>{stable_scaling, sort_by::scaled} {
  }
};

template<chromosome Chromosome,
         fitness Raw,
         fitness Scaled = empty_fitness,
         typename Tags = empty_tags>
class population {
public:
  using chromosome_t = Chromosome;
  using raw_fitness_t = Raw;
  using scaled_fitness_t = Scaled;

  using individual_t =
      individual<chromosome_t, raw_fitness_t, scaled_fitness_t, Tags>;

  using collection_t = std::vector<individual_t>;

  using iterator_t = typename collection_t::iterator;
  using const_iterator_t = typename collection_t::const_iterator;

public:
  inline explicit population(bool stable_scaling) noexcept
      : target_size_{std::nullopt}
      , stable_scaling_{stable_scaling} {
  }

  inline explicit population(std::size_t target_size, bool stable_scaling)
      : target_size_{target_size}
      , stable_scaling_{stable_scaling} {
    if (target_size_) {
      individuals_.reserve(*target_size_);
    }
  }

  template<std::ranges::range Range>
  inline auto insert(Range&& chromosomes) {
    auto insertion = individuals_.size();

    sorted_ = sort_by::none;

    auto output = std::back_inserter(individuals_);
    if constexpr (std::is_reference_v<Range>) {
      std::ranges::copy(chromosomes, output);
    }
    else {
      std::ranges::move(std::move(chromosomes), output);
    }

    return std::ranges::views::drop(individuals_, insertion);
  }

  template<replacement_range<iterator_t, individual_t> Range>
  auto replace(Range&& replacements) {
    auto removed = ensure_removed(std::ranges::size(replacements));

    sorted_ = sort_by::none;

    for (auto& replacement : replacements) {
      auto parent = get_parent(replacement);

      removed.emplace_back(std::move(*parent));

      if constexpr (std::is_reference_v<Range>) {
        *parent = get_child(replacement);
      }
      else {
        *parent = std::move(get_child(replacement));
      }
    }

    return removed;
  }

  inline auto trim() {
    if (target_size_ && *target_size_ < individuals_.size()) {
      return trim_impl(*target_size_);
    }

    return {};
  }

  inline auto trim(std::size_t to_trim) {
    return trim_impl(
        to_trim < individuals_.size() ? individuals_.size() - to_trim : 0);
  }

  template<sort_fitness_tag<raw_fitness_t, scaled_fitness_t> FitnessTag>
  inline void sort(FitnessTag /*unused*/) {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (sorted_ != policy.by) {
      sorted_ = sort_by::none;
      policy.sort(individuals_);
      sorted_ = policy.by;
    }
  }

  template<sort_fitness_tag<raw_fitness_t, scaled_fitness_t> FitnessTag>
  inline std::pair<individual_t const&, individual_t const&>
      extremes(FitnessTag /*unused*/) const noexcept {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (sorted_ == policy.by) {
      return {individuals_.back(), individuals_.front()};
    }
    else {
      auto [mini, maxi] = policy.minmax(individuals_);
      return {*mini, *maxi};
    }
  }

  inline auto& individuals() noexcept {
    return individuals_;
  }

  inline auto const& individuals() const noexcept {
    return individuals_;
  }

  inline std::size_t current_size() const noexcept {
    return individuals_.size();
  }

  inline auto target_size() const noexcept {
    return target_size_;
  }

private:
  auto trim_impl(std::size_t size) {
    auto removed = ensure_removed(individuals_.size() - size);

    std::ranges::move(std::ranges::views::drop(individuals_, size),
                      std::back_inserter(removed));

    individuals_.resize(size);

    return removed;
  }

  inline auto ensure_removed(std::size_t size) const {
    collection_t result{};
    result.reserve(size);

    return result;
  }

private:
  std::optional<std::size_t> target_size_;
  collection_t individuals_;

  sort_by sorted_{sort_by::none};
  bool stable_scaling_;
};

template<typename FitnessTag>
struct get_fitness_type;

template<>
struct get_fitness_type<raw_fitness_tag> {
  template<typename Population>
  using type = typename Population::raw_fitness_t;
};

template<>
struct get_fitness_type<scaled_fitness_tag> {
  template<typename Population>
  using type = typename Population::scaled_fitness_t;
};

template<typename FitnessType, typename Population>
using get_fitness_t =
    typename get_fitness_type<FitnessType>::template type<Population>;

template<typename Population, typename FitnessTag>
concept ordered_population =
    ordered_fitness<get_fitness_t<FitnessTag, Population>>;

template<typename Population, typename FitnessTag>
concept averageable_population =
    arithmetic_fitness<get_fitness_t<FitnessTag, Population>>;

} // namespace gal
