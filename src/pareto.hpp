
#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

namespace gal {
namespace pareto {

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

  } // namespace details

  template<typename Impl>
  class solution {
  public:
    inline explicit solution(Impl* impl) noexcept
        : impl_{impl} {
    }

    inline auto& individual() const noexcept {
      return impl_->individual();
    }

    inline auto dominated() const noexcept {
      return impl_->dominated() |
             std::views::transform([](Impl* item) { return solution{item}; });
    }

    inline auto dominators_total() const noexcept {
      return impl_->dominators_total();
    }

  private:
    Impl* impl_;
  };

  namespace details {

    template<typename Individual>
    class solution_impl : public solution<solution_impl<Individual>> {
    public:
      using individual_t = Individual;

    public:
      inline explicit solution_impl(individual_t& individual)
          : individual_{individual} {
      }

      inline void add_dominated(solution_impl* dominated) {
        dominated_.push_back(dominated);
        dominated->inc_dominators();
      }

      inline void dec_dominators() noexcept {
        return --dominators_left_ == 0;
      }

      inline bool in_frontier() const noexcept {
        return dominators_left_ == 0;
      }

      inline auto& individual() noexcept {
        return individual_;
      }

      inline auto const& individual() const noexcept {
        return individual_;
      }

      inline auto& dominated() noexcept {
        return dominated_;
      }

      inline auto const& dominated() const noexcept {
        return dominated_;
      }

      inline auto dominators_total() const noexcept {
        return dominators_total_;
      }

    private:
      inline void inc_dominators() noexcept {
        ++dominators_total_;
        ++dominators_left_;
      }

    private:
      individual_t* individual_;

      std::size_t dominators_total_;
      std::size_t dominators_left_;
      std::vector<solution_impl*> dominated_;
    };

  } // namespace details

  template<typename Impl>
  class frontier {
  public:
    inline explicit frontier(Impl* impl) noexcept
        : impl_{&impl} {
    }

    inline auto level() const noexcept {
      return impl_.level();
    }

    inline auto members() const noexcept {
      return impl_->members() |
             std::views::transform([](Impl* item) { return frontier{item}; });
    }

    inline auto empty() const noexcept {
      return impl_->empty();
    }

  private:
    Impl* impl_;
  };

  namespace details {

    template<typename Individual>
    class frontier_impl : public frontier<frontier_impl<Individual>> {
    public:
      using individual_t = Individual;
      using members_t = std::vector<details::solution_impl<individual_t>*>;

    public:
      template<std::ranges::range Members>
        requires std::same_as<individual_t, std::ranges::range_value_t<Members>>
      inline explicit frontier_impl(std::size_t level, Members&& members)
          : level_{level}
          , members_{std::ranges::begin(members), std::ranges::end(members)} {
      }

      inline explicit frontier_impl(std::size_t level, members_t&& members)
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

    template<typename Individual>
    class state {
    public:
      using individual_t = Individual;
      using frontier_t = frontier_impl<individual_t>;

    private:
      using solution_t = solution_impl<individual_t>;
      using members_t = std::vector<solution_t*>;

      using collection_t = std::vector<frontier_t>;

    public:
      using frontiers_iterator_t = typename collection_t::iterator;

    public:
      template<typename Range, typename Comparator>
      inline explicit state(Range&& range, Comparator const& comparator)
          : solutions_{wrap(std::forward<Range>(range))} {
        compare_all(comparator);
      }

      inline auto begin() {
        return frontiers_.empty() ? identify_first() : frontiers_.begin();
      }

      inline auto next(frontiers_iterator_t it) {
        auto n = std::next(it);
        return completed_ || n != frontiers_.end() ? n : identify_next(it);
      }

    private:
      template<typename Range>
      inline auto wrap(Range&& range) {
        auto transformed =
            range | std::views::transform([](individual_t& individual) {
              return solution_t{individual};
            });

        return std::vector<solution_t>{std::ranges::begin(transformed),
                                       std::ranges::end(transformed)};
      }

      template<typename Comparator>
      inline void compare_all(Comparator const& comparator) {
        std::ranges::for_each(
            details::paired_iterator{std::ranges::begin(solutions_),
                                     std::ranges::end(solutions_)},
            std::ranges::end(solutions_),
            [&comparator](std::pair<solution_t, solution_t>& pair) {
              compare_pair(pair, comparator);
            });
      }

      template<typename Comparator>
      inline void compare_pair(std::pair<solution_t, solution_t>& pair,
                               Comparator const& comparator) {
        auto& [fst, snd] = pair;

        auto result = std::invoke(
            comparator, fst.individual().raw(), snd.individual().raw());

        if (result == std::weak_ordering::greater) {
          fst.add_dominated(snd);
        }
        else if (result == std::weak_ordering::less) {
          snd.add_dominated(fst);
        }
      }

      void identify_first() {
        auto frontier = frontiers_.emplace_back(
            0, solutions_ | std::views::filter([](auto const& item) {
                 return item.in_frontier();
               }));

        completed_ = frontier.empty();

        return frontiers_.begin();
      }

      void identify_next(frontiers_iterator_t it) {
        members_t members{};

        for (auto& dominator : it->members()) {
          for (auto& dominated : dominator.dominated()) {
            if (dominated.dec_dominators()) {
              members.push_back(&dominated);
            }
          }
        }

        if (members.empty()) {
          completed_ = true;
          return frontiers_.end();
        }

        frontiers_.emplace_back(frontiers_.size(), std::move(members));
        return frontiers_.back();
      }

    private:
      std::vector<solution_t> solutions_;
      collection_t frontiers_;
      bool completed_{false};
    };

  } // namespace details

  struct frontiers_sentinel {};

  template<typename Individual>
  class frontiers_iterator {
  private:
    using individual_t = Individual;
    using state_t = details::state<individual_t>;

    using frontiers_iterator_t = typename state_t::frontiers_iterator_t;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = frontier<individual_t>;
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

    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

  public:
    inline explicit frontiers_iterator(state_t& state) noexcept
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
      return value_type{&*base_};
    }

    inline pointer operator->() const {
      return pointer{**this};
    }

    template<typename Ty>
    friend bool operator==(frontiers_iterator<Ty> const&,
                           frontiers_iterator<Ty> const&) noexcept;

    template<typename Ty>
    friend bool operator==(frontiers_iterator<Ty> const&,
                           frontiers_sentinel) noexcept;

  private:
    state_t* state_;
    frontiers_iterator_t base_;
  };

  template<typename Ty>
  inline bool operator==(frontiers_iterator<Ty> const& lhs,
                         frontiers_iterator<Ty> const& rhs) noexcept {
    // todo: implement
    return false;
  }

  template<typename Ty>
  inline bool operator!=(frontiers_iterator<Ty> const& lhs,
                         frontiers_iterator<Ty> const& rhs) noexcept {
    return !(lhs == rhs);
  }

  template<typename Ty>
  inline bool operator==(frontiers_iterator<Ty> const& lhs,
                         frontiers_sentinel rhs) noexcept {
    // todo: implement
    return false;
  }

  template<typename Ty>
  inline bool operator!=(frontiers_iterator<Ty> const& lhs,
                         frontiers_sentinel rhs) noexcept {
    return !(lhs == rhs);
  }

  template<typename Ty>
  inline bool operator==(frontiers_sentinel lhs,
                         frontiers_iterator<Ty> const& rhs) noexcept {
    return rhs == lhs;
  }

  template<typename Ty>
  inline bool operator!=(frontiers_sentinel rhs,
                         frontiers_iterator<Ty> const& lhs) noexcept {
    return !(lhs == rhs);
  }

  template<std::ranges::forward_range Source, typename Comparator>
    requires std::ranges::view<Source>
  class sorted_view
      : public std::ranges::view_interface<sorted_view<Source, Comparator>> {
  private:
    using individual_t = std::ranges::range_value_t<Source>;

  public:
    using comparator_t = Comparator;
    using iterator_t = frontiers_iterator<individual_t>;

  public:
    sorted_view() = default;

    inline explicit sorted_view(Source source, comparator_t const& comparator)
        : state_{std::forward<Source>(source), comparator} {
    }

    inline auto begin() const {
      return iterator_t{state_};
    }

    inline auto end() const noexcept {
      return frontiers_sentinel{};
    }

  private:
    mutable details::state<individual_t> state_;
  };

  namespace details {

    template<typename Comparator>
    class sorted_view_closure {
    public:
      inline explicit sorted_view_closure(Comparator const& comparator)
          : comparator_{comparator} {
      }

      inline explicit sorted_view_closure(Comparator&& comparator)
          : comparator_{std::move(comparator)} {
      }

      template<std::ranges::viewable_range Range>
      inline auto operator()(Range&& range) {
        return sorted_view{std::forward<Range>(range), comparator_};
      }

    private:
      Comparator comparator_;
    };

    struct sorted_view_adaptor {
      template<std::ranges::viewable_range Range, typename Comparator>
      inline auto operator()(Range&& range, Comparator&& comparator) {
        return sorted_view{std::forward<Range>(range),
                           std::forward<Comparator>(comparator)};
      }

      template<typename Comparator>
      inline auto operator()(Comparator&& comparator) {
        return sorted_view_closure{std::forward<Comparator>(comparator)};
      }
    };

    template<std::ranges::viewable_range Range, typename Comparator>
    inline auto operator|(Range&& range,
                          sorted_view_closure<Comparator> const& closure) {
      return closure(std::forward<Range>(range));
    }

  } // namespace details

  namespace views {
    inline details::sorted_view_adaptor sorted;
  }

} // namespace pareto
} // namespace gal
