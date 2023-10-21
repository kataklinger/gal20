
#pragma once

#include "individual.hpp"

#include <algorithm>
#include <vector>

namespace gal {

template<typename FitnessTag, typename Raw, typename Scaled>
concept sort_fitness_tag = ( std::same_as<FitnessTag, raw_fitness_tag> &&
                             !std::same_as<Raw, disabled_comparator> ) ||
                           (std::same_as<FitnessTag, scaled_fitness_tag> &&
                            !std::same_as<Scaled, disabled_comparator>);

enum class sort_by { none, raw, scaled, both };

template<typename FitnessTag>
struct sort_policy_base {
  sort_by const by;

  inline explicit sort_policy_base(bool stable_scaling, sort_by value) noexcept
      : by{stable_scaling ? sort_by::both : value} {
  }

  template<typename Collection, typename Comparator>
  inline auto minmax(Collection const& individuals,
                     Comparator&& compare) noexcept {
    return std::ranges::minmax_element(
        individuals, std::forward<Comparator>(compare), [](auto const& i) {
          return i.evaluation().get(FitnessTag{});
        });
  }

  template<typename Collection, typename Comparator>
  inline void sort(Collection& individuals, Comparator&& compare) {
    std::ranges::sort(
        individuals, std::forward<Comparator>(compare), [](auto const& i) {
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
         comparator<Raw> RawCompare,
         fitness Scaled,
         comparator<Scaled> ScaledCompare,
         typename Tags>
class population {
public:
  using chromosome_t = Chromosome;

  using raw_fitness_t = Raw;
  using raw_comparator_t = RawCompare;

  using scaled_fitness_t = Scaled;
  using scaled_comparator_t = ScaledCompare;

  using individual_t =
      individual<chromosome_t, raw_fitness_t, scaled_fitness_t, Tags>;

  using collection_t = std::vector<individual_t>;

  using iterator_t = typename collection_t::iterator;
  using const_iterator_t = typename collection_t::const_iterator;

public:
  inline population(raw_comparator_t const& raw_comparator,
                    scaled_comparator_t const& scaled_comparator,
                    bool stable_scaling) noexcept
      : raw_comparator_{raw_comparator}
      , scaled_comparator_{scaled_comparator}
      , target_size_{std::nullopt}
      , stable_scaling_{stable_scaling} {
  }

  inline population(raw_comparator_t const& raw_comparator,
                    scaled_comparator_t const& scaled_comparator,
                    std::size_t target_size,
                    bool stable_scaling)
      : raw_comparator_{raw_comparator}
      , scaled_comparator_{scaled_comparator}
      , target_size_{target_size}
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
    std::ranges::move(std::move(chromosomes), output);

    return std::views::drop(individuals_, insertion);
  }

  template<replacement_range<iterator_t, individual_t> Range>
  auto replace(Range&& replacements) {
    auto removed = ensure_removed(std::ranges::size(replacements));

    sorted_ = sort_by::none;

    for (auto&& replace_pair : replacements) {
      auto parent = get_parent(replace_pair);

      removed.emplace_back(std::move(*parent));

      *parent = std::move(get_child(replace_pair));
    }

    return removed;
  }

  inline auto trim() {
    if (target_size_ && *target_size_ < individuals_.size()) {
      return trim_impl(*target_size_);
    }

    return collection_t{};
  }

  inline auto trim_to(std::size_t size) {
    return trim_impl(size);
  }

  inline auto trim(std::size_t to_trim) {
    return trim_impl(
        to_trim < individuals_.size() ? individuals_.size() - to_trim : 0);
  }

  inline auto trim_all() {
    return trim_impl(0);
  }

  template<sort_fitness_tag<raw_comparator_t, scaled_comparator_t> FitnessTag>
  inline void sort(FitnessTag tag) {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (sorted_ != policy.by) {
      sorted_ = sort_by::none;
      policy.sort(individuals_, comparator(tag));
      sorted_ = policy.by;
    }
  }

  template<sort_fitness_tag<raw_comparator_t, scaled_comparator_t> FitnessTag>
  inline std::pair<individual_t const&, individual_t const&>
      extremes(FitnessTag tag) const noexcept {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (sorted_ == policy.by) {
      return {individuals_.back(), individuals_.front()};
    }
    else {
      auto [mini, maxi] = policy.minmax(individuals_, comparator(tag));
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

  inline auto const& raw_comparator() const noexcept {
    return raw_comparator_;
  }

  inline auto const& scaled_comparator() const noexcept {
    return scaled_comparator_;
  }

  inline auto const& comparator(raw_fitness_tag /*unused*/) const noexcept {
    return raw_comparator_;
  }

  inline auto const& comparator(scaled_fitness_tag /*unused*/) const noexcept {
    return scaled_comparator_;
  }

private:
  auto trim_impl(std::size_t size) {
    auto removed = ensure_removed(individuals_.size() - size);

    std::ranges::move(std::views::drop(individuals_, size),
                      std::back_inserter(removed));

    individuals_.erase(individuals_.begin() + size, individuals_.end());

    return removed;
  }

  inline auto ensure_removed(std::size_t size) const {
    collection_t result{};
    result.reserve(size);

    return result;
  }

private:
  raw_comparator_t raw_comparator_;
  scaled_comparator_t scaled_comparator_;

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

template<typename FitnessTag>
struct get_fitness_comparator;

template<>
struct get_fitness_comparator<raw_fitness_tag> {
  template<typename Population>
  using type = typename Population::raw_comparator_t;
};

template<>
struct get_fitness_comparator<scaled_fitness_tag> {
  template<typename Population>
  using type = typename Population::scaled_comparator_t;
};

template<typename FitnessType, typename Population>
using get_fitness_comparator_t =
    typename get_fitness_comparator<FitnessType>::template type<Population>;

// clang-format off

template<typename Population, typename FitnessTag>
concept ordered_population =
    !std::same_as<get_fitness_comparator_t<FitnessTag, Population>,
                  disabled_comparator>;

// clang-format on

template<typename Population>
concept multiobjective_population =
    multiobjective_fitness<typename Population::raw_fitness_t>;

template<typename Population, typename FitnessTag>
concept averageable_population =
    arithmetic_fitness<get_fitness_t<FitnessTag, Population>>;

namespace details {

  template<typename Tuple, typename Tag>
  struct has_tag_impl : std::false_type {};

  template<typename Tag, typename... Tys>
  struct has_tag_impl<std::tuple<Tys...>, Tag>
      : std::conjunction<std::is_same<Tag, Tys>...> {};

  template<typename Population, typename Tag>
  struct is_tagged_with_single
      : std::disjunction<std::is_same<typename Population::Tags, Tag>,
                         has_tag_impl<typename Population::Tags, Tag>> {};

} // namespace details

template<typename Population, typename... Tags>
struct is_tagged_with
    : std::conjunction<details::is_tagged_with_single<Population, Tags>...> {};

template<typename Population, typename... Tags>
inline constexpr auto is_tagged_with_v =
    is_tagged_with<Population, Tags...>::value;

template<typename Population, typename... Tags>
concept tagged_with = is_tagged_with_v<Population, Tags...>;

} // namespace gal