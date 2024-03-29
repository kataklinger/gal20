
#pragma once

#include "individual.hpp"

#include <algorithm>
#include <vector>

namespace gal {

enum class sort_by { none, raw, scaled, both };

template<typename FitnessTag>
struct sort_policy_base {
  inline static constexpr FitnessTag fitness_tag{};

  sort_by const by;

  inline explicit sort_policy_base(bool stable_scaling, sort_by value) noexcept
      : by{stable_scaling ? sort_by::both : value} {
  }

  template<typename Collection, typename Comparator>
  inline auto minmax(Collection const& individuals,
                     Comparator&& compare) noexcept {
    return std::ranges::minmax_element(
        individuals,
        fitness_worse{std::forward<Comparator>(compare)},
        [](auto const& i) { return i.eval().get(fitness_tag); });
  }

  template<typename Collection, typename Comparator>
  inline auto sort(Collection& individuals, Comparator&& compare) {
    std::ranges::sort(individuals,
                      fitness_better{std::forward<Comparator>(compare)},
                      [](auto const& i) { return i.eval().get(fitness_tag); });

    return by;
  }

  inline auto sorted(sort_by current) const noexcept {
    return current == by;
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

  template<typename FitnessTag>
    requires(std::same_as<FitnessTag, raw_fitness_tag> ||
             std::same_as<FitnessTag, scaled_fitness_tag>)
  using fitness_ordering_t = typename std::conditional_t<
      std::is_same_v<FitnessTag, raw_fitness_tag>,
      comparator_traits<raw_comparator_t, raw_fitness_t>,
      comparator_traits<scaled_comparator_t, scaled_fitness_t>>::ordering_t;

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
  inline auto insert(Range&& individuals) {
    auto insertion = individuals_.size();

    as_unsorted();

    auto output = std::back_inserter(individuals_);
    std::ranges::move(std::move(individuals), output);

    return std::views::drop(individuals_, insertion);
  }

  template<forward_replacement_range<iterator_t, individual_t> Range>
  auto replace(Range&& replacements) {
    auto removed = ensure_removed(replacements);

    as_unsorted();

    for (auto&& replace_pair : replacements) {
      auto parent = get_parent(replace_pair);

      removed.emplace_back(std::move(*parent));

      *parent = std::move(get_child(replace_pair));
    }

    return removed;
  }

  template<std::predicate<individual_t const&> Pred>
  inline void remove_if(Pred&& predicate) noexcept {
    std::erase_if(individuals_, std::forward<Pred>(predicate));
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

  template<typename FitnessTag>
    requires(
        std::convertible_to<fitness_ordering_t<FitnessTag>, std::weak_ordering>)
  inline void sort(FitnessTag tag) {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (!policy.sorted(sorted_)) {
      as_unsorted();
      sorted_ = policy.sort(individuals_, comparator(tag));
    }
  }

  template<std::predicate<individual_t const&, individual_t const&> Less>
  inline void sort(Less&& less) {
    as_unsorted();

    std::ranges::sort(individuals_, std::forward<Less>(less));
  }

  template<typename FitnessTag>
    requires(
        std::convertible_to<fitness_ordering_t<FitnessTag>, std::weak_ordering>)
  inline std::pair<individual_t const&, individual_t const&>
      extremes(FitnessTag tag) const noexcept {
    sort_policy<FitnessTag> policy{stable_scaling_};

    if (policy.sorted(sorted_)) {
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

  inline auto adopted_raw_comparator() const noexcept {
    return [adopted = raw_comparator_](individual_t const& lhs,
                                       individual_t const& rhs) {
      return std::invoke(adopted, lhs.eval().raw(), rhs.eval().raw());
    };
  }

  inline auto const& scaled_comparator() const noexcept {
    return scaled_comparator_;
  }

  inline auto adopted_scaled_comparator() const noexcept {
    return [adopted = scaled_comparator_](individual_t const& lhs,
                                          individual_t const& rhs) {
      return std::invoke(adopted, lhs.eval().scaled(), rhs.eval().scaled());
    };
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

  template<std::ranges::sized_range Range>
  inline auto ensure_removed(Range const& range) {
    return ensure_removed(std::ranges::size(range));
  }

  template<std::ranges::range Range>
  inline auto ensure_removed(Range const& /*unused*/) {
    return collection_t{};
  }

  inline void as_unsorted() noexcept {
    sorted_ = sort_by::none;
  }

private:
  raw_comparator_t raw_comparator_;
  scaled_comparator_t scaled_comparator_;

  std::optional<std::size_t> target_size_;
  collection_t individuals_;

  sort_by sorted_{};
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

template<typename FitnessType, typename Population>
using population_ordering =
    typename Population::template fitness_ordering_t<FitnessType>;

template<typename Population, typename FitnessTag>
concept sortable_population =
    std::convertible_to<population_ordering<FitnessTag, Population>,
                        std::weak_ordering>;

template<typename Population>
concept multiobjective_population =
    multiobjective_fitness<typename Population::raw_fitness_t>;

template<typename Population>
concept spatial_population =
    spatial_fitness<typename Population::raw_fitness_t>;

template<typename Population>
concept grid_population = grid_fitness<typename Population::raw_fitness_t>;

template<typename Population, typename FitnessTag>
concept averageable_population =
    averageable_fitness<get_fitness_t<FitnessTag, Population>>;

template<typename Population, typename... Tags>
concept population_tagged_with =
    is_individual_tagged_with_v<typename Population::individual_t, Tags...>;

template<std::default_initializable... CleanedTags,
         chromosome Chromosome,
         fitness Raw,
         comparator<Raw> RawCompare,
         fitness Scaled,
         comparator<Scaled> ScaledCompare,
         typename AllTags>
inline void clean_tags(
    population<Chromosome, Raw, RawCompare, Scaled, ScaledCompare, AllTags>&
        population)
  requires(std::conjunction_v<details::has_tag_impl<AllTags, CleanedTags>...>)
{
  for (auto&& individual : population.individuals()) {
    ((get_tag<CleanedTags>(individual) = {}), ...);
  }
}

template<typename Factory, typename Context>
using operation_factory_result_t =
    std::invoke_result_t<std::add_const_t<Factory>,
                         std::add_lvalue_reference_t<Context>>;

} // namespace gal
