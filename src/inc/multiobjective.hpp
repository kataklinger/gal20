
#pragma once

#include "pareto.hpp"
#include "population.hpp"

#include <cassert>

namespace gal {

template<typename Value>
concept adapted_value = std::regular<Value> && std::totally_ordered<Value>;

template<typename Tag, adapted_value Value>
class tag_adapted_value {
public:
  using tag_t = Tag;
  using value_t = Value;

public:
  inline tag_adapted_value() noexcept(
      std::is_nothrow_default_constructible_v<value_t>) {
  }

  inline tag_adapted_value(value_t value) noexcept(
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

private:
  value_t value_;
};

struct rank_tag {};

enum class binary_rank : std::uint8_t { nondominated, dominated, undefined };

inline auto operator<=>(binary_rank lhs, binary_rank rhs) noexcept {
  using type = std::underlying_type_t<binary_rank>;
  return static_cast<type>(lhs) <=> static_cast<type>(rhs);
}

using bin_rank_t = tag_adapted_value<rank_tag, binary_rank>;
using int_rank_t = tag_adapted_value<rank_tag, std::size_t>;
using real_rank_t = tag_adapted_value<rank_tag, double>;

struct frontier_level_tag {};
using frontier_level_t =
    tag_adapted_value<frontier_level_tag, pareto::frontier_level>;

namespace details {

  template<typename Population, typename... Tags>
  concept mo_tagged_population = multiobjective_population<Population> &&
                                 population_tagged_with<Population, Tags...>;

}

template<typename Population, typename RankTag>
concept ranked_population =
    details::mo_tagged_population<Population, frontier_level_t, RankTag>;

template<typename BaseIterator>
class pareto_sets_iterator {
private:
  using base_iterator_t = BaseIterator;
  using inner_iterator_t = typename base_iterator_t::value_type;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = std::ranges::subrange<inner_iterator_t>;
  using reference = value_type;

  struct pointer {
    value_type value;

    inline auto* operator->() noexcept {
      return &value;
    }

    inline auto const* operator->() const noexcept {
      return &value;
    }
  };

  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;

public:
  inline explicit pareto_sets_iterator(base_iterator_t base) noexcept
      : base_{base} {
  }

  inline auto& operator++() {
    ++base_;
    return *this;
  }

  inline auto operator++(int) {
    pareto_sets_iterator ret{base_};
    ++base_;
    return ret;
  }

  inline auto operator*() const {
    return value_type{*base_, *(base_ + 1)};
  }

  inline pointer operator->() const {
    return {**this};
  }

  template<typename Ty>
  friend bool operator==(pareto_sets_iterator<Ty> const&,
                         pareto_sets_iterator<Ty> const&) noexcept;

private:
  base_iterator_t base_;
};

template<typename Ty>
inline bool operator==(pareto_sets_iterator<Ty> const& lhs,
                       pareto_sets_iterator<Ty> const& rhs) noexcept {
  return lhs.base_ == rhs.base_;
}

template<typename Ty>
inline bool operator!=(pareto_sets_iterator<Ty> const& lhs,
                       pareto_sets_iterator<Ty> const& rhs) noexcept {
  return !(lhs == rhs);
}

struct pareto_preserved_t {};
inline constexpr pareto_preserved_t pareto_preserved_tag{};

struct pareto_reduced_t {};
inline constexpr pareto_reduced_t pareto_reduced_tag{};

struct pareto_nondominated_t {};
inline constexpr pareto_nondominated_t pareto_nondominated_tag{};

struct pareto_erased_t {};
inline constexpr pareto_erased_t pareto_erased_tag{};

template<typename Preserved>
concept pareto_preservance = std::same_as<Preserved, pareto_preserved_t> ||
                             std::same_as<Preserved, pareto_reduced_t> ||
                             std::same_as<Preserved, pareto_nondominated_t> ||
                             std::same_as<Preserved, pareto_erased_t>;

template<typename Individual, pareto_preservance Preserved>
class pareto_sets {
public:
  using individual_t = Individual;
  using preserved_t = Preserved;

private:
  using individuals_t = std::vector<individual_t*>;
  using set_boundery = typename std::vector<individual_t*>::iterator;

public:
  using iterator_t =
      pareto_sets_iterator<typename std::vector<set_boundery>::iterator>;

public:
  inline explicit pareto_sets(std::size_t max_individuals)
      : max_individuals_{max_individuals} {
    individuals_.reserve(max_individuals);
    set_boundaries_.push_back(individuals_.begin());
  }

  inline auto begin() const {
    return iterator_t{set_boundaries_.begin()};
  }

  inline auto end() const noexcept {
    return iterator_t{set_boundaries_.end()};
  }

  inline void trim() noexcept {
    if (set_boundaries_.size() > 1) {
      individuals_.erase(set_boundaries_[1], individuals_.end());
      set_boundaries_.resize(1);
    }
  }

  inline void add_individual(individual_t& individual) {
    if constexpr (!std::is_same_v<preserved_t, pareto_erased_t>) {
      if constexpr (std::is_same_v<preserved_t, pareto_nondominated_t>) {
        if (set_boundaries_.size() > 1) {
          return;
        }
      }

      assert(individual.size() < max_individuals_);

      individuals_.push_back(&individual);
    }
  }

  inline void next() {
    if constexpr (!std::is_same_v<preserved_t, pareto_erased_t>) {
      if constexpr (std::is_same_v<preserved_t, pareto_reduced_t> ||
                    std::is_same_v<preserved_t, pareto_nondominated_t>) {
        if (set_boundaries_.size() > 1) {
          return;
        }
      }

      set_boundaries_.push_back(individuals_.end());
    }
  }

  inline void finish() {
    if constexpr (std::is_same_v<preserved_t, pareto_reduced_t>) {
      if (set_boundaries_.back() != individuals_.end()) {
        set_boundaries_.push_back(individuals_.end());
      }
    }
  }

  inline auto count() const noexcept {
    set_boundaries_.size() - 1;
  }

  inline auto get_count_of(pareto::frontier_level level) {
    return set_boundaries_[level] - set_boundaries_[level - 1];
  }

  inline auto empty() const noexcept {
    return set_boundaries_.size() <= 1;
  }

  inline auto at(pareto::frontier_level level) const noexcept {
    return std::ranges::subrange{set_boundaries_[level - 1],
                                 set_boundaries_[level]};
  }

  inline auto operator[](pareto::frontier_level level) const noexcept {
    return at(level);
  }

private:
  std::size_t max_individuals_;
  std::vector<individual_t*> individuals_;
  std::vector<set_boundery> set_boundaries_;
};

template<multiobjective_population Population, typename Preserved>
using population_pareto_t =
    pareto_sets<typename Population::individual_t, Preserved>;

template<typename Operation, typename Population, typename Preserved>
concept ranking = std::is_invocable_r_v<
    population_pareto_t<Population, Preserved>,
    Operation,
    std::add_lvalue_reference_t<Population>,
    std::add_lvalue_reference_t<std::add_const_t<Preserved>>>;

class cluster_label {
private:
  inline constexpr explicit cluster_label(std::size_t raw,
                                          int /*unused*/) noexcept
      : raw_{raw} {
  }

public:
  inline static constexpr auto unique() noexcept {
    return cluster_label{1, 0};
  }

  inline static constexpr auto unassigned() noexcept {
    return cluster_label{0, 0};
  }

  cluster_label() = default;

  inline constexpr explicit cluster_label(std::size_t index) noexcept
      : raw_{(index << 1) | 1} {
  }

  inline auto is_proper() const noexcept {
    return (raw_ & 1) == 1;
  }

  inline auto is_unique() const noexcept {
    return (raw_ & 1) == 0 && raw_ > 0;
  }

  inline auto is_unassigned() const noexcept {
    return raw_ == 0;
  }

  inline auto index() const noexcept {
    assert(is_proper());

    return raw_ >> 1;
  }

private:
  std::size_t raw_{};
};

template<typename Population>
concept clustered_population =
    details::mo_tagged_population<Population, cluster_label>;

class cluster_set {
public:
  struct cluster {
    std::size_t level_;
    std::size_t members_;
  };

public:
  inline auto add_cluster(std::size_t members) {
    clusters_.emplace_back(level_, members);
    return cluster_label{clusters_.size() - 1};
  }

  inline auto add_cluster() {
    return add_cluster(0);
  }

  inline void next_level() noexcept {
    ++level_;
  }

  inline void add_member(std::size_t cluster_index) noexcept {
    ++clusters_[cluster_index].members_;
  }

  inline auto begin() noexcept {
    return clusters_.begin();
  }

  inline auto end() noexcept {
    return clusters_.end();
  }

  inline auto begin() const noexcept {
    return clusters_.begin();
  }

  inline auto end() const noexcept {
    return clusters_.end();
  }

  inline auto cbegin() noexcept {
    return clusters_.cbegin();
  }

  inline auto cend() noexcept {
    return clusters_.cend();
  }

  inline auto size() noexcept {
    return clusters_.size();
  }

  inline auto& operator[](std::size_t cluster_index) noexcept {
    return clusters_[cluster_index];
  }

  inline auto const& operator[](std::size_t cluster_index) const noexcept {
    return clusters_[cluster_index];
  }

private:
  std::vector<cluster> clusters_;
  std::size_t level_{};
};

template<typename Operation, typename Population, typename Preserved>
concept clustering = std::is_invocable_r_v<
    cluster_set,
    Operation,
    std::add_lvalue_reference_t<Population>,
    std::add_lvalue_reference_t<population_pareto_t<Population, Preserved>>>;

struct crowd_tag {};

using crowd_density_t = tag_adapted_value<crowd_tag, double>;

template<typename Population>
concept crowded_population =
    details::mo_tagged_population<Population, crowd_density_t>;

template<typename Operation, typename Population, typename Preserved>
concept crowading = std::invocable<
    Operation,
    std::add_lvalue_reference_t<Population>,
    std::add_lvalue_reference_t<population_pareto_t<Population, Preserved>>,
    cluster_set const&>;

struct prune_tag {};

using prune_state_t = tag_adapted_value<prune_tag, bool>;

template<typename Population>
concept prunable_population =
    details::mo_tagged_population<Population, prune_state_t>;

namespace details {
  template<typename Operation, typename Population, typename Preserved>
  concept cluster_pruning_helper =
      std::invocable<Operation,
                     std::add_lvalue_reference_t<Population>,
                     cluster_set&>;

  template<typename Operation, typename Population, typename Preserved>
  concept crowd_pruning_helper =
      std::invocable<Operation, std::add_lvalue_reference_t<Population>>;
} // namespace details

template<typename Operation, typename Population, typename Preserved>
concept cluster_pruning =
    details::cluster_pruning_helper<Operation, Population, Preserved> &&
    !details::crowd_pruning_helper<Operation, Population, Preserved>;

template<typename Operation, typename Population, typename Preserved>
concept crowd_pruning =
    details::crowd_pruning_helper<Operation, Population, Preserved> &&
    !details::cluster_pruning_helper<Operation, Population, Preserved>;

template<typename Operation, typename Population, typename Preserved>
concept pruning =
    details::cluster_pruning_helper<Operation, Population, Preserved> ||
    details::crowd_pruning_helper<Operation, Population, Preserved>;

namespace details {

  template<typename Population,
           typename Preserved,
           pruning<Population, Preserved> Pruning>
  struct pruning_helper;

  template<typename Population,
           typename Preserved,
           cluster_pruning<Population, Preserved> Pruning>
  struct pruning_helper<Population, Preserved, Pruning> {
    inline static constexpr auto cluster_based = true;
    inline static constexpr auto crowd_based = false;
  };

  template<typename Population,
           typename Preserved,
           crowd_pruning<Population, Preserved> Pruning>
  struct pruning_helper<Population, Preserved, Pruning> {
    inline static constexpr auto cluster_based = false;
    inline static constexpr auto crowd_based = true;
  };

} // namespace details

template<typename Population, typename Preserved, typename Pruning>
struct pruning_traits
    : details::pruning_helper<Population, Preserved, Pruning> {};

template<typename Operation, typename Population, typename Preserved>
concept projection = std::invocable<
    Operation,
    std::add_lvalue_reference_t<population_pareto_t<Population, Preserved>>,
    cluster_set const&>;

} // namespace gal
