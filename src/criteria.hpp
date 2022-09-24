
#pragma once

#include "statistic.hpp"

namespace gal {
namespace criteria {

  template<typename History, typename Model>
  concept tracked_history =
      gal::stat::tracked_models<typename History::statistics_t, Model>;

  class generation {
  public:
    inline explicit generation(std::size_t limit)
        : limit_{limit} {
    }

    template<typename Population, tracked_history<stat::generation> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) const noexcept {
      return stat::get_generation{}(history.current()) > limit_;
    }

  private:
    std::size_t limit_;
  };

  template<typename Getter, typename Comparer>
  class value_limit {
  public:
    using getter_t = Getter;
    using comparer_t = Comparer;

    using model_t = typename Getter::model_t;

  public:
    inline explicit value_limit(getter_t const& getter,
                                comparer_t const& comparer)
        : getter_{getter}
        , comparer_{comparer} {
    }

    template<typename Population, tracked_history<model_t> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) const {
      return comparer_(getter_(history.current()));
    }

  private:
    getter_t getter_;
    comparer_t comparer_;
  };

  template<typename Getter>
  class value_progress {
  public:
    using getter_t = Getter;

    using model_t = typename Getter::model_t;

  public:
    inline explicit value_progress(getter_t const& getter, std::size_t limit)
        : getter_{getter}
        , limit_{limit} {
    }

    template<typename Population, tracked_history<model_t> History>
    inline bool operator()(Population const& /*unused*/,
                           History const& history) {
      stagnated_ = getter_(history.current()) > getter_(history.previous())
                       ? 0
                       : stagnated_ + 1;

      return stagnated_ >= limit_;
    }

  private:
    getter_t getter_;
    std::size_t limit_;
    std::size_t stagnated_{};
  };

} // namespace criteria
} // namespace gal