
#pragma once

#include "population.hpp"

namespace gal {

template<typename Value>
concept rank_value = std::regular<Value> && std::totally_ordered<Value>;

template<rank_value Value>
class rank_tag {
public:
  using value_t = Value;

public:
  inline rank_tag() noexcept(std::is_nothrow_default_constructible_v<value_t>) {
  }

  inline rank_tag(value_t value) noexcept(
      std::is_nothrow_copy_constructible_v<value_t>)
      : value_{value} {
  }

  inline operator value_t() const
      noexcept(std::is_nothrow_copy_constructible_v<value_t>) {
    return value_;
  }

  inline rank_tag& operator=(value_t value) noexcept(
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

enum class binary_rank : std::uint8_t { nondominated, dominated, undefined };

inline auto operator<=>(binary_rank lhs, binary_rank rhs) noexcept {
  using type = std::underlying_type_t<binary_rank>;
  return static_cast<type>(lhs) <=> static_cast<type>(rhs);
}

using bin_rank_t = rank_tag<binary_rank>;
using int_rank_t = rank_tag<std::size_t>;
using real_rank_t = rank_tag<double>;

template<typename Population, typename RankTag>
concept ranked_population =
    multiobjective_population<Population> && tagged_with<Population, RankTag>;

template<typename Individual>
class pareto_sets {
public:
  using individual_t = Individual;

private:
  using individuals_t = std::vector<individual_t*>;
  using set_boundery = typename std::vector<individual_t*>::iterator;

public:
  inline explicit pareto_sets(std::size_t max_individuals)
      : max_individuals_{max_individuals} {
    individuals_.reserve(max_individuals);
  }

  inline void add_inidividual(individual_t& individual) {
    assert(individuals_.size() < max_individuals_);
    individuals_.push_back(&individual);
  }

  inline void next() {
    set_boundaries_.push_back(individuals_.end());
  }

  inline auto count() const noexcept {
    set_boundaries_.size() + 1;
  }

  inline auto at(std::size_t level) const noexcept {
    return std::ranges::subrange{level == 0 ? individuals_.begin()
                                            : set_boundaries_[level - 1],
                                 set_boundaries_[level]};
  }

  inline auto operator[](std::size_t level) const noexcept {
    return at(level);
  }

private:
  std::size_t max_individuals_;
  std::vector<individual_t*> individuals_;
  std::vector<set_boundery> set_boundaries_;
};

} // namespace gal
