
#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <vector>

namespace gal {
namespace pareto {

  namespace details {

    template<std::forward_iterator Base>
    class paired_iterator {
    public:
      using base_iterator_t = Base;

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

        inline auto* operator->() noexcept {
          return &value;
        }

        inline auto const* operator->() const noexcept {
          return &value;
        }
      };

    public:
      paired_iterator() = default;

      inline explicit paired_iterator(base_iterator_t last) noexcept
          : i_{last}
          , j_{last}
          , last_{last} {
      }

      inline explicit paired_iterator(base_iterator_t first,
                                      base_iterator_t last) noexcept
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

      template<typename Ty>
      friend bool operator==(paired_iterator<Ty> const&,
                             paired_iterator<Ty> const&) noexcept;

    private:
      base_iterator_t i_{};
      base_iterator_t j_{};
      base_iterator_t last_{};
    };

    template<typename Ty>
    inline bool operator==(paired_iterator<Ty> const& lhs,
                           paired_iterator<Ty> const& rhs) noexcept {
      return lhs.i_ == rhs.i_ && lhs.j_ == rhs.j_ && lhs.last_ == rhs.last_;
    }

    template<typename Ty>
    inline bool operator!=(paired_iterator<Ty> const& lhs,
                           paired_iterator<Ty> const& rhs) noexcept {
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
    class solution_impl {
    public:
      using individual_t = Individual;

    public:
      inline explicit solution_impl(individual_t& individual) noexcept
          : individual_{&individual} {
      }

      inline void add_dominated(solution_impl& dominated) {
        dominated_.push_back(&dominated);
        dominated.inc_dominators();
      }

      inline auto dec_dominators() noexcept {
        return --dominators_left_ == 0;
      }

      inline auto in_frontier() const noexcept {
        return dominators_left_ == 0;
      }

      inline auto& individual() noexcept {
        return *individual_;
      }

      inline auto const& individual() const noexcept {
        return *individual_;
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

      std::size_t dominators_total_{};
      std::size_t dominators_left_{};
      std::vector<solution_impl*> dominated_;
    };

  } // namespace details

  template<typename Impl>
  class frontier {
  public:
    inline explicit frontier(Impl* impl) noexcept
        : impl_{impl} {
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

    template<typename Range>
    using get_individual_t =
        std::ranges::range_value_t<std::remove_reference_t<Range>>;

    template<typename Range>
    inline auto wrap_all(Range&& range) {
      using individual_t = get_individual_t<Range>;
      using solution_t = solution_impl<individual_t>;

      auto transformed =
          range | std::views::transform([](individual_t& individual) {
            return solution_t{individual};
          });

      return std::vector<solution_t>{std::ranges::begin(transformed),
                                     std::ranges::end(transformed)};
    }

    template<typename Pair, typename Comparator>
    inline static void compare_pair(Pair& pair, Comparator&& comparator) {
      auto& [fst, snd] = pair;

      auto result = std::invoke(std::forward<Comparator>(comparator),
                                fst.individual().evaluation().raw(),
                                snd.individual().evaluation().raw());

      if (result == std::weak_ordering::greater) {
        fst.add_dominated(snd);
      }
      else if (result == std::weak_ordering::less) {
        snd.add_dominated(fst);
      }
    }

    template<typename Solutions, typename Comparator>
    inline void compare_all(Solutions& solutions, Comparator comparator) {
      std::ranges::for_each(
          details::paired_iterator{std::ranges::begin(solutions),
                                   std::ranges::end(solutions)},
          details::paired_iterator{std::ranges::end(solutions)},
          [&comparator](auto pair) { compare_pair(pair, comparator); });
    }

    template<typename Range, typename Comparator>
    auto analyze_impl(Range&& range, Comparator&& comparator) {
      auto results = wrap_all(std::forward<Range>(range));
      compare_all(results, std::forward<Comparator>(comparator));
      return results;
    }

    template<typename Individual>
    class frontier_impl {
    public:
      using individual_t = Individual;
      using member_t = details::solution_impl<individual_t>*;
      using members_t = std::vector<member_t>;

    public:
      template<std::ranges::range Members>
        requires std::same_as<member_t, std::ranges::range_value_t<Members>>
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

      using solution_pair_t = std::pair<solution_t&, solution_t&>;

      using collection_t = std::vector<frontier_t>;

    public:
      using frontiers_iterator_t = typename collection_t::iterator;

    public:
      template<typename Range, typename Comparator>
      inline explicit state(Range&& range, Comparator&& comparator)
          : solutions_{analyze_impl(std::forward<Range>(range),
                                    std::forward<Comparator>(comparator))} {
      }

      inline auto begin() {
        return frontiers_.empty() ? identify_first() : frontiers_.begin();
      }

      inline auto next(frontiers_iterator_t it) {
        auto n = std::next(it);
        return completed_ || n != frontiers_.end() ? n : identify_next(it);
      }

      inline bool is_last(frontiers_iterator_t it) {
        return it == frontiers_.end() && completed_;
      }

    private:
      auto identify_first() {
        auto filtered = solutions_ | std::views::filter([](auto const& item) {
                          return item.in_frontier();
                        }) |
                        std::views::transform([](auto& item) { return &item; });

        auto frontier = frontiers_.emplace_back(0, filtered);

        completed_ = frontier.empty();

        return frontiers_.begin();
      }

      auto identify_next(frontiers_iterator_t it) {
        members_t members{};

        for (auto dominator : it->members()) {
          for (auto dominated : dominator->dominated()) {
            if (dominated->dec_dominators()) {
              members.push_back(dominated);
            }
          }
        }

        if (members.empty()) {
          completed_ = true;
          return frontiers_.end();
        }

        frontiers_.emplace_back(frontiers_.size(), std::move(members));
        return --frontiers_.end();
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
    using value_type = frontier<details::frontier_impl<individual_t>>;
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
      return {**this};
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
    return lhs.state_ == rhs.state_ && lhs.base_ == rhs.base_;
  }

  template<typename Ty>
  inline bool operator!=(frontiers_iterator<Ty> const& lhs,
                         frontiers_iterator<Ty> const& rhs) noexcept {
    return !(lhs == rhs);
  }

  template<typename Ty>
  inline bool operator==(frontiers_iterator<Ty> const& lhs,
                         frontiers_sentinel /*unused*/) noexcept {
    return lhs.state_->is_last(lhs.base_);
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

  template<std::ranges::forward_range Range, typename Comparator>
    requires std::ranges::view<Range>
  class sort_view
      : public std::ranges::view_interface<sort_view<Range, Comparator>> {
  private:
    using individual_t = std::ranges::range_value_t<Range>;
    using state_t = details::state<individual_t>;

  public:
    using comparator_t = Comparator;
    using iterator_t = frontiers_iterator<individual_t>;

  public:
    sort_view() = default;

    inline sort_view(Range range, comparator_t const& comparator)
        : state_{std::make_shared<state_t>(std::move(range), comparator)} {
    }

    inline auto begin() const {
      return iterator_t{*state_};
    }

    inline auto end() const noexcept {
      return frontiers_sentinel{};
    }

  private:
    std::shared_ptr<state_t> state_;
  };

  template<typename Range, typename Comparator>
  sort_view(Range&& range, Comparator&&)
      -> sort_view<std::views::all_t<Range>, std::remove_cvref_t<Comparator>>;

  namespace details {

    template<typename Comparator>
    class sort_view_closure {
    public:
      inline explicit sort_view_closure(Comparator const& comparator)
          : comparator_{comparator} {
      }

      inline explicit sort_view_closure(Comparator&& comparator)
          : comparator_{std::move(comparator)} {
      }

      template<std::ranges::viewable_range Range>
      inline auto operator()(Range&& range) const {
        return sort_view{std::forward<Range>(range), comparator_};
      }

    private:
      Comparator comparator_;
    };

    struct sort_view_adaptor {
      template<std::ranges::viewable_range Range, typename Comparator>
      inline auto operator()(Range&& range, Comparator&& comparator) const {
        return sort_view{std::forward<Range>(range),
                         std::forward<Comparator>(comparator)};
      }

      template<typename Comparator>
      inline auto operator()(Comparator&& comparator) const {
        return sort_view_closure{std::forward<Comparator>(comparator)};
      }
    };

    template<std::ranges::viewable_range Range, typename Comparator>
    inline auto operator|(Range&& range,
                          sort_view_closure<Comparator> const& closure) {
      return closure(std::forward<Range>(range));
    }

  } // namespace details

  namespace views {
    inline details::sort_view_adaptor sort;
  }

  template<std::ranges::viewable_range Range, typename Comparator>
  inline auto analyze(Range&& range, Comparator&& comparator) {
    return details::analyze_impl(std::forward<Range>(range),
                                 std::forward<Comparator>(comparator)) |
           std::views::transform([](auto& item) { return solution{&item}; });
  }

  namespace details {

    template<typename ItOut, typename Range, typename Comparator>
    void identify_inner(ItOut outer,
                        Range& inner,
                        std::vector<bool>::iterator flag_out,
                        std::vector<bool>::iterator flag_in,
                        std::vector<ItOut>& output,
                        Comparator& comparator) {
      for (auto it = std::ranges::begin(inner), end = std::ranges::end(inner);
           it != end;
           ++it) {
        if (!*(flag_in++)) {
          auto result = std::invoke(
              comparator, outer->evaluation().raw(), it->evaluation().raw());

          if (result == std::weak_ordering::greater) {
            *flag_in = true;
            output.push_back(outer);
          }
          else if (result == std::weak_ordering::less) {
            *flag_out = true;
            output.push_back(it);
            break;
          }
        }
      }
    }

  } // namespace details

  template<std::ranges::sized_range Range, typename Comparator>
  auto identify_dominated(
      Range&& range,
      std::ranges::iterator_t<std::remove_reference_t<Range>> fence,
      Comparator comparator) {
    std::vector<bool> identified(std::ranges::size(range));
    std::vector<decltype(fence)> dominated{};

    auto i = std::begin(identified);
    auto j = i + std::distance(std::ranges::begin(range), fence);

    auto out = std::ranges::begin(range);
    auto end = std::ranges::end(range);

    for (std::ranges::subrange in{fence, end}; out != fence; ++out) {
      details::identify_inner(out, in, i++, j, dominated, comparator);
    }

    for (auto it = out; out != end; ++out, ++i) {
      std::ranges::subrange in{++it, end};
      details::identify_inner(out, in, i, i + 1, dominated, comparator);
    }

    return dominated;
  }

} // namespace pareto
} // namespace gal
