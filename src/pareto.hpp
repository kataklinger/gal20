
#pragma once

#include <iterator>
#include <vector>

namespace gal {

namespace details {

  template<std::forward_iterator BaseIt>
  class pareto_iter {
  public:
    using base_iterator_t = BaseIt;

  private:
    using iter_traits_t = std::iterator_traits<base_iterator_t>;
    using base_value_ref_t =
        std::add_lvalue_reference_t<typename iter_traits_t::value_type>;

  public:
    using difference_type = typename iter_traits_t::difference_type;
    using value_type = std::pair<base_value_ref_t, base_value_ref_t>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

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
    pareto_iter() = default;

    inline explicit pareto_iter(base_iterator_t last) noexcept
        : i_{last}
        , j_{last}
        , last_{last} {
    }

    inline explicit pareto_iter(base_iterator_t first,
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

    template<typename T>
    friend bool operator==(pareto_iter<T> const&,
                           pareto_iter<T> const&) noexcept;

  private:
    base_iterator_t i_{};
    base_iterator_t j_{};
    base_iterator_t last_{};
  };

  template<std::forward_iterator BaseIt>
  inline bool operator==(pareto_iter<BaseIt> const& lhs,
                         pareto_iter<BaseIt> const& rhs) noexcept {
    return lhs.i_ == rhs.i_ && lhs.j_ == rhs.j_ && lhs.last_ == rhs.last_;
  }

  template<std::forward_iterator BaseIt>
  inline bool operator!=(pareto_iter<BaseIt> const& lhs,
                         pareto_iter<BaseIt> const& rhs) noexcept {
    return !(lhs == rhs);
  }

} // namespace details

template<std::forward_iterator It>
void pareto_sort(It first, It last) {
}

} // namespace gal
