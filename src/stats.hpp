
#pragma once

#include "population.hpp"

#include <concepts>
#include <numeric>
#include <type_traits>

namespace gal {
namespace stats {

  template<typename Value, typename Population>
  concept node_value =
      !std::is_final_v<typename Value::template type<Population>> &&
      std::is_constructible_v<typename Value::template type<Population>,
                              Population const&,
                              typename Value::template type<Population> const&>;

  namespace details {

    template<typename Population, node_value<Population>... Values>
    class node;

    template<typename Population>
    class node<Population> {};

    template<typename Population, node_value<Population> Value>
    class node<Population, Value> : public Value::template type<Population> {
    public:
      using value_t = typename Value::template type<Population>;

    public:
      template<typename Population>
      inline node(Population const& population,
                  node<Population> const& previous)
          : value_t{population, previous} {
      }
    };

    template<typename Population,
             node_value<Population> Value,
             node_value<Population>... Rest>
    class node<Population, Value, Rest...>
        : public Value::template type<Population>,
          public node<Population, Rest...> {
    public:
      using value_t = typename Value::template type<Population>;
      using base_t = node<Population, Rest...>;

    public:
      template<typename Population>
      inline node(Population const& population, node const& previous)
          : value_t{population, previous}
          , base_t{previous} {
      }
    };

    template<arithmetic_fintess Value>
    class kahan_state {
    public:
      using value_t = Value;

    public:
      inline kahan_state() = default;

      inline auto add(value_t const& value) const noexcept {
        value_t y = value - correction_;
        value_t t = sum_ + y;
        return kahan_state{t, (t - sum_) - y};
      }

      inline value_t const& sum() const noexcept {
        return sum_;
      }

    private:
      inline kahan_state(value_t const& sum, value_t const& correction)
          : sum_{sum}
          , correction_{correction} {
      }

    private:
      value_t sum_{};
      value_t correction_{};
    };

  } // namespace details

  struct generation {
    template<typename Population>
    class type {
    public:
      inline type(Population const& population, type const& previous) noexcept
          : value_{previous.value_ + 1} {
      }

      inline auto generation_value() const noexcept {
        return value_;
      }

    private:
      std::size_t value_{};
    };
  };

  struct population_size {
    template<typename Population>
    class type {
    public:
      inline type(Population const& population, type const& previous) noexcept
          : value_{population.current_size()} {
      }

      inline auto population_size_value() const noexcept {
        return value_;
      }

    private:
      std::size_t value_{};
    };
  };

  template<typename FitnessTag>
  struct extreme_fitness {
    template<ordered_population<FitnessTag> Population>
    class type {
    private:
      using fitness_tag_t = FitnessTag;

      using fitness_t = get_fitness_t<FitnessTag, Population>;
      using minmax_fitness_t = std::pair<fitness_t, fitness_t>;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      inline type(Population const& population, type const& previous) noexcept {
        auto [mini, maxi] = population.extremes(fitness_tag);
        value_ = minmax_fitness_t{mini.evaluation().get(fitness_tag),
                                  maxi.evaluation().get(fitness_tag)};
      }

      inline auto const& worst_fitness_value() const noexcept {
        return value_.first;
      }

      inline auto const& best_fitness_value() const noexcept {
        return value_.second;
      }

    private:
      minmax_fitness_t value_{};
    };
  };

  template<typename FitnessTag>
  struct average_fitness {
    template<averageable_population<FitnessTag> Population>
    class type {
    private:
      using fitness_tag_t = FitnessTag;

      using fitness_t = get_fitness_t<FitnessTag, Population>;
      using state_t = details::kahan_state<fitness_t>;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      inline type(Population const& population, type const& previous) noexcept
          : value_{std::accumulate(population.individuals().begin(),
                                   population.individuals().end(),
                                   state_t{},
                                   [](state_t acc, auto const& ind) {
                                     return acc.add(
                                         ind.evaluation().get(fitness_tag));
                                   })
                       .sum() /
                   population.current_size()} {
      }

      inline auto const& average_value() const noexcept {
        return value_;
      }

    private:
      fitness_t value_;
    };
  };

  template<typename Population, node_value<Population>... Values>
  class statistics : public details::node<Population, Values...> {
  private:
    using base_t = details::node<Population, Values...>;

  private:
    inline statistics(Population const& population, statistics const& previous)
        : base_t{population, previous} {
    }

  public:
    inline auto next(Population const& population) const {
      return statistics(population, *this);
    }
  };

} // namespace stats
} // namespace gal
