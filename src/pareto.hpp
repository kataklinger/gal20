
#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

namespace gal {

namespace details {

  template<std::forward_iterator Base, std::sentinel_for<Base> Sentinel>
  class paired_iterator {
  public:
    using base_iterator_t = Base;
    using sentinel_t = Sentinel;

  private:
    using iter_traits_t = std::iterator_traits<base_iterator_t>;
    using base_value_ref_t =
        std::add_lvalue_reference_t<typename iter_traits_t::value_type>;

  public:
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    using difference_type = typename iter_traits_t::difference_type;
    using value_type = std::pair<base_value_ref_t, base_value_ref_t>;
    using reference = value_type;

    struct pointer {
      value_type value;

      inline value_type* operator->() noexcept {
        return &value;
      }

      inline value_type const* operator->() const noexcept {
        return &value;
      }
    };

  public:
    paired_iterator() = default;

    inline explicit paired_iterator(base_iterator_t first,
                                    sentinel_t last) noexcept
        : i_{first}
        , j_{first}
        , last_{last} {
      ++j_;
    }

    inline auto& operator++() noexcept {
      if (++j_ == last_) {
        j_ = ++i_;
        ++j_;
      }

      return *this;
    }

    inline auto operator++(int) noexcept {
      auto ret = *this;
      ++*this;
      return ret;
    }

    inline auto operator*() const noexcept {
      return value_type{*i_, *j_};
    }

    inline pointer operator->() const noexcept {
      return {**this};
    }

    template<typename Tx, typename Ty>
    friend bool operator==(paired_iterator<Tx, Ty> const&,
                           paired_iterator<Tx, Ty> const&) noexcept;

    template<typename Tx, typename Ty>
    friend bool operator==(paired_iterator<Tx, Ty> const&, Ty) noexcept;

  private:
    base_iterator_t i_{};
    base_iterator_t j_{};
    sentinel_t last_{};
  };

  template<typename Tx, typename Ty>
  inline bool operator==(paired_iterator<Tx, Ty> const& lhs,
                         paired_iterator<Tx, Ty> const& rhs) noexcept {
    return lhs.i_ == rhs.i_ && lhs.j_ == rhs.j_ && lhs.last_ == rhs.last_;
  }

  template<typename Tx, typename Ty>
  inline bool operator!=(paired_iterator<Tx, Ty> const& lhs,
                         paired_iterator<Tx, Ty> const& rhs) noexcept {
    return !(lhs == rhs);
  }

  template<typename Tx, typename Ty>
  inline bool operator==(paired_iterator<Tx, Ty> const& lhs, Ty rhs) {
    return lhs.i_ == rhs && lhs.j_ == rhs;
  }

  template<typename Tx, typename Ty>
  inline bool operator!=(paired_iterator<Tx, Ty> const& lhs, Ty rhs) {
    return !(lhs == rhs);
  }

  template<typename Tx, typename Ty>
  inline bool operator==(Ty lhs, paired_iterator<Tx, Ty> const& rhs) {
    return rhs == lhs;
  }

  template<typename Tx, typename Ty>
  inline bool operator!=(Ty lhs, paired_iterator<Tx, Ty> const& rhs) {
    return !(lhs == rhs);
  }

  template<typename Individual>
  class paretor_wrapper {
  public:
    using individual_t = Individual;

  public:
    inline explicit paretor_wrapper(individual_t& individual)
        : individual_{individual} {
    }

    inline void add_dominated(paretor_wrapper* dominated) {
      dominated_.push_back(dominated);
      ++dominated->dominators_;
    }

    inline void clear_dominator() noexcept {
      return --dominators_ == 0;
    }

    inline auto& dominated() noexcept {
      return dominated_;
    }

    inline auto const& dominated() const noexcept {
      return dominated_;
    }

    inline auto dominators() const noexcept {
      return dominators_;
    }

    inline bool nondominated() const noexcept {
      return dominators_ == 0;
    }

    inline auto const& fitness() const noexcept {
      return individual_->raw();
    }

  private:
    individual_t* individual_;

    std::size_t dominators_;
    std::vector<paretor_wrapper*> dominated_;
  };

} // namespace details

template<typename Individual>
class pareto_front {
public:
  using individual_t = Individual;
  using members_t = std::vector<details::paretor_wrapper<individual_t>*>;

public:
  template<std::ranges::range Members>
    requires std::same_as<individual_t, std::ranges::range_value_t<Members>>
  inline explicit pareto_front(std::size_t level, Members&& members)
      : level_{level}
      , members_{std::ranges::begin(members), std::ranges::end(members)} {
  }

  inline explicit pareto_front(std::size_t level, members_t&& members)
      : level_{level}
      , members_{std::move(members)} {
  }

  inline auto level() const noexcept {
    return level_;
  }

  inline auto& members() noexcept {
    return members_;
  }

  inline auto const& members() const noexcept {
    return members_;
  }

  inline auto empty() const noexcept {
    return members_.empty();
  }

private:
  std::size_t level_;
  members_t members_;
};

namespace details {

  template<typename Individual>
  class pareto_state {
  public:
    using individual_t = Individual;
    using front_t = pareto_front<individual_t>;

  private:
    using wrapper_t = paretor_wrapper<individual_t>;
    using members_t = std::vector<wrapper_t*>;

    using collection_t = std::vector<front_t>;

  public:
    using fronts_iterator_t = typename collection_t::iterator;

  public:
    template<typename Range, typename Comparator>
    inline explicit pareto_state(Range&& range, Comparator const& comparator)
        : individuals_{wrap(std::forward<Range>(range))} {
      compare_all(comparator);
    }

    inline auto begin() {
      return fronts_.empty() ? identify_first() : fronts_.begin();
    }

    inline auto next(fronts_iterator_t it) {
      auto n = std::next(it);
      return completed_ || n != fronts_.end() ? n : identify_next(it);
    }

  private:
    template<typename Range>
    inline auto wrap(Range&& range) {
      auto transformed =
          range | std::ranges::views::transform([](individual_t& individual) {
            return wrapper_t{individual};
          });

      return std::vector<wrapper_t>{std::ranges::begin(transformed),
                                    std::ranges::end(transformed)};
    }

    template<typename Comparator>
    inline void compare_all(Comparator const& comparator) {
      std::ranges::for_each(
          details::paired_iterator{std::ranges::begin(individuals_),
                                   std::ranges::end(individuals_)},
          std::ranges::end(individuals_),
          [&comparator](std::pair<wrapper_t, wrapper_t>& pair) {
            compare_pair(pair, comparator);
          });
    }

    template<typename Comparator>
    inline void compare_pair(std::pair<wrapper_t, wrapper_t>& pair,
                             Comparator const& comparator) {
      auto& [fst, snd] = pair;

      auto result = std::invoke(comparator, fst.fitness(), snd.fitness());
      if (result == std::weak_ordering::greater) {
        fst.add_dominated(snd);
      }
      else if (result == std::weak_ordering::less) {
        snd.add_dominated(fst);
      }
    }

    void identify_first() {
      auto front = fronts_.emplace_back(
          0, individuals_ | std::ranges::views::filter([](auto const& ind) {
               return ind.nondominated();
             }));

      completed_ = front.empty();

      return fronts_.begin();
    }

    void identify_next(fronts_iterator_t it) {
      members_t members{};

      for (auto& dominator : it->members()) {
        for (auto& dominated : dominator.dominated()) {
          if (dominated.clear_dominator()) {
            members.push_back(&dominated);
          }
        }
      }

      if (members.empty()) {
        completed_ = true;
        return fronts_.end();
      }

      fronts_.emplace_back(fronts_.size(), std::move(members));
      return fronts_.back();
    }

  private:
    std::vector<wrapper_t> individuals_;
    collection_t fronts_;
    bool completed_{false};
  };

} // namespace details

struct pareto_sentinel {};

template<typename Individual>
class pareto_iterator {
private:
  using individual_t = Individual;
  using state_t = details::pareto_state<individual_t>;

  using fronts_iterator_t = typename state_t::fronts_iterator_t;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = pareto_front<individual_t>;
  using reference = value_type&;
  using pointer = value_type*;

  using iterator_category = std::forward_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;

public:
  inline explicit pareto_iterator(state_t& state) noexcept
      : state_{&state}
      , base_{state.begin()} {
  }

  inline auto& operator++() {
    base_ = state_->next(base_);
    return *this;
  }

  inline auto operator++(int) {
    auto ret = *this;
    ++*this;
    return ret;
  }

  inline auto operator*() const {
    return *base_;
  }

  inline pointer operator->() const {
    return &**this;
  }

  template<typename Ty>
  friend bool operator==(pareto_iterator<Ty> const&,
                         pareto_iterator<Ty> const&) noexcept;

  template<typename Ty>
  friend bool operator==(pareto_iterator<Ty> const&, pareto_sentinel) noexcept;

private:
  state_t* state_;
  fronts_iterator_t base_;
};

template<typename Ty>
inline bool operator==(pareto_iterator<Ty> const& lhs,
                       pareto_iterator<Ty> const& rhs) noexcept {
  // todo: implement
  return false;
}

template<typename Ty>
inline bool operator!=(pareto_iterator<Ty> const& lhs,
                       pareto_iterator<Ty> const& rhs) noexcept {
  return !(lhs == rhs);
}

template<typename Ty>
inline bool operator==(pareto_iterator<Ty> const& lhs,
                       pareto_sentinel rhs) noexcept {
  // todo: implement
  return false;
}

template<typename Ty>
inline bool operator!=(pareto_iterator<Ty> const& lhs,
                       pareto_sentinel rhs) noexcept {
  return !(lhs == rhs);
}

template<typename Ty>
inline bool operator==(pareto_sentinel lhs,
                       pareto_iterator<Ty> const& rhs) noexcept {
  return rhs == lhs;
}

template<typename Ty>
inline bool operator!=(pareto_sentinel rhs,
                       pareto_iterator<Ty> const& lhs) noexcept {
  return !(lhs == rhs);
}

template<std::ranges::forward_range Source, typename Comparator>
  requires std::ranges::view<Source>
class pareto_view
    : public std::ranges::view_interface<pareto_view<Source, Comparator>> {
private:
  using individual_t = std::ranges::range_value_t<Source>;

public:
  using comparator_t = Comparator;
  using iterator_t = pareto_iterator<individual_t>;

public:
  pareto_view() = default;

  inline explicit pareto_view(Source source, comparator_t const& comparator)
      : state_{std::forward<Source>(source), comparator} {
  }

  inline auto begin() const {
    return iterator_t{state_};
  }

  inline auto end() const noexcept {
    return pareto_sentinel{};
  }

private:
  mutable details::pareto_state<individual_t> state_;
};

namespace details {

  template<typename Comparator>
  class pareto_view_closure {
  public:
    inline explicit pareto_view_closure(Comparator const& comparator)
        : comparator_{comparator} {
    }

    inline explicit pareto_view_closure(Comparator&& comparator)
        : comparator_{std::move(comparator)} {
    }

    template<std::ranges::viewable_range Range>
    inline auto operator()(Range&& range) {
      return pareto_view(std::forward<Range>(range), comparator_);
    }

  private:
    Comparator comparator_;
  };

  struct pareto_view_adaptor {
    template<std::ranges::viewable_range Range, typename Comparator>
    inline auto operator()(Range&& range, Comparator&& comparator) {
      return pareto_view(std::forward<Range>(range),
                         std::forward<Comparator>(comparator));
    }

    template<typename Comparator>
    inline auto operator()(Comparator&& comparator) {
      return pareto_view_closure(std::forward<Comparator>(comparator));
    }
  };

  template<std::ranges::viewable_range Range, typename Comparator>
  inline auto operator|(Range&& range,
                        pareto_view_closure<Comparator> const& closure) {
    return closure(std::forward<Range>(range));
  }

} // namespace details

namespace views {
  inline details::pareto_view_adaptor pareto;
}

} // namespace gal
