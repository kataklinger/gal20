
#pragma once

#include "statistics.hpp"

namespace gal {
namespace criteria {

  template<typename History, typename Model>
  concept tracked_history =
      stats::tracked_models<typename History::statistics_t, Model>;

  class generation_limit {
  public:
    inline explicit generation_limit(std::size_t limit)
        : limit_{limit} {
    }

    template<typename Population, tracked_history<stats::generation> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) const noexcept {
      return stats::get_generation{}(history.current()) > limit_;
    }

  private:
    std::size_t limit_;
  };

  template<typename Getter, typename Comparator>
  class value_limit {
  public:
    using getter_t = Getter;
    using comparator_t = Comparator;

    using model_t = typename Getter::model_t;

  public:
    inline explicit value_limit(getter_t const& getter,
                                comparator_t const& compare)
        : getter_{getter}
        , comparator_{compare} {
    }

    template<typename Population, tracked_history<model_t> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) const {
      return std::invoke(comparator_, std::invoke(getter_, history.current()));
    }

  private:
    getter_t getter_;
    comparator_t comparator_;
  };

  template<typename Getter, typename Comparator>
  class value_progress {
  public:
    using getter_t = Getter;
    using comparator_t = Comparator;

    using model_t = typename Getter::model_t;

  public:
    inline explicit value_progress(getter_t const& getter,
                                   comparator_t const& compare,
                                   std::size_t limit)
        : getter_{getter}
        , comparator_{compare}
        , limit_{limit} {
    }

    template<typename Population, tracked_history<model_t> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) {
      stagnated_ = std::invoke(comparator_,
                               std::invoke(getter_, history.current()),
                               std::invoke(getter_, history.previous()))
                       ? 0
                       : stagnated_ + 1;

      return stagnated_ >= limit_;
    }

  private:
    getter_t getter_;
    comparator_t comparator_;

    std::size_t limit_;
    std::size_t stagnated_{};
  };

} // namespace criteria
} // namespace gal
