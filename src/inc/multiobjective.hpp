
#pragma once

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

namespace details {

  template<typename Population, typename... Tags>
  concept mo_tagged_population = multiobjective_population<Population> &&
                                 population_tagged_with<Population, Tags...>;

}

template<typename Population, typename RankTag>
concept ranked_population = details::mo_tagged_population<Population, RankTag>;

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

template<typename Individual>
class pareto_sets {
public:
  using individual_t = Individual;

private:
  using individuals_t = std::vector<individual_t*>;
  using set_boundery = typename std::vector<individual_t*>::iterator;
  using set_boundaries_t = std::vector<set_boundery>;

public:
  using iterator_t = pareto_sets_iterator<typename set_boundaries_t::iterator>;

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

  inline void add_inidividual(individual_t& individual) {
    assert(individuals_.size() < max_individuals_);
    individuals_.push_back(&individual);
  }

  inline void next() {
    set_boundaries_.push_back(individuals_.end());
  }

  inline auto count() const noexcept {
    set_boundaries_.size() - 1;
  }

  inline auto at(std::size_t level) const noexcept {
    return std::ranges::subrange{set_boundaries_[level],
                                 set_boundaries_[level + 1]};
  }

  inline auto operator[](std::size_t level) const noexcept {
    return at(level);
  }

private:
  std::size_t max_individuals_;
  std::vector<individual_t*> individuals_;
  set_boundaries_t set_boundaries_;
};

template<multiobjective_population Population>
using population_pareto_t = pareto_sets<typename Population::individual_t>;

struct pareto_preserved_t {};
inline constexpr pareto_preserved_t pareto_preserved_tag{};

struct pareto_reduced_t {};
inline constexpr pareto_reduced_t pareto_reduced_tag{};

struct pareto_nondominated_t {};
inline constexpr pareto_nondominated_t pareto_nondominated_tag{};

struct pareto_erased_t {};
inline constexpr pareto_erased_t pareto_erased_tag{};

namespace details {

  template<typename Operation, typename Population, typename PreserveTag>
  concept ranking_base = std::is_invocable_r_v<
      population_pareto_t<Population>,
      Operation,
      std::add_lvalue_reference_t<Population>,
      std::add_lvalue_reference_t<std::add_const_t<PreserveTag>>>;

} // namespace details

template<typename Operation, typename Population>
concept ranking =
    details::ranking_base<Operation, Population, pareto_preserved_t> &&
    details::ranking_base<Operation, Population, pareto_reduced_t> &&
    details::ranking_base<Operation, Population, pareto_nondominated_t> &&
    details::ranking_base<Operation, Population, pareto_erased_t>;

class niche_label {
public:
  constexpr niche_label() = default;

  inline constexpr niche_label(std::size_t front, std::size_t group) noexcept
      : front_{front}
      , group_{group} {
  }

  inline constexpr explicit niche_label(std::size_t front) noexcept
      : front_{front}
      , group_{0} {
  }

  inline auto front() const noexcept {
    return front_ - 1;
  }

  inline auto group() const noexcept {
    return group_ - 1;
  }

  inline auto is_proper() const noexcept {
    return front_ != 0 && group_ != 0;
  }

  inline auto is_unassigned() const noexcept {
    return front_ == 0 && group_ == 0;
  }

  inline auto is_unique() const noexcept {
    return front_ != 0 && group_ == 0;
  }

private:
  std::size_t front_{};
  std::size_t group_{};
};

struct niche_tag {};

using niche_density_t = tag_adapted_value<niche_tag, double>;

template<typename Population>
concept niched_population =
    details::mo_tagged_population<Population, niche_density_t, niche_label>;

class niche_membership {
public:
  inline niche_membership(niche_label label, std::size_t member_count) noexcept
      : label_{label}
      , member_count_{member_count} {
  }

  inline void add_member() noexcept {
    ++member_count_;
  }

  inline auto const& label() const noexcept {
    return label_;
  }

  inline auto member_count() const noexcept {
    return member_count_;
  }

private:
  niche_label label_;
  std::size_t member_count_{};
};

class niche_set {
public:
  inline auto& add_front() noexcept {
    ++fronts_count_;
    return *this;
  }

  inline auto& add_niche(std::size_t member_count) {
    assert(fronts_count_ > 0);

    return niches_.emplace_back(niche_label{fronts_count_, niches_.size() + 1},
                                member_count);
  }

  inline auto& operator[](niche_label const& label) noexcept {
    return niches_[label.group() - 1];
  }

  inline auto const& operator[](niche_label const& label) const noexcept {
    return niches_[label.group() - 1];
  }

  inline auto unique_label() const noexcept {
    assert(fronts_count_ > 0);

    return niche_label{fronts_count_, 0};
  }

  inline auto fronts_count() const noexcept {
    return fronts_count_;
  }

  inline auto niches_count() const noexcept {
    return niches_.size();
  }

private:
  std::size_t fronts_count_{};
  std::vector<niche_membership> niches_;
};

template<typename Operation, typename Population>
concept niching = std::is_invocable_r_v<
    niche_set,
    Operation,
    std::add_lvalue_reference_t<Population>,
    std::add_lvalue_reference_t<population_pareto_t<Population>>>;

template<typename Operation, typename Population>
concept projection = std::invocable<Operation, niche_set const&>;

} // namespace gal
