
#pragma once

#include "pop.hpp"

#include <chrono>
#include <concepts>
#include <numeric>
#include <queue>
#include <tuple>
#include <type_traits>

namespace gal {
namespace stats {

  template<typename... Dependencies>
  struct dependencies {};

  template<typename Population, typename Dependencies>
  struct dependencies_pack;

  template<typename Population, typename... Dependencies>
  struct dependencies_pack<Population, dependencies<Dependencies...>> {
    template<typename Dependency>
    using element_t = std::add_lvalue_reference_t<
        std::add_const_t<typename Dependency::template type<Population>>>;

    using type = std::tuple<element_t<Dependencies>...>;

    template<typename Source>
    auto pack(Source const& source) {
      return type{static_cast<element_t<Dependencies>>(source)...};
    }

    template<typename Dependency>
    decltype(auto) unpack(type const& pack) {
      return std::get<element_t<Dependency>>(pack);
    }
  };

  template<typename Pack, typename Dependency, typename Dependencies>
  decltype(auto) unpack_dependency(Dependencies const& dependencies) {
    return Pack::template unpack<Dependency>(dependencies);
  }

  namespace details {

    template<typename Value, typename = void>
    struct node_value_dependencies {
      using type = dependencies<>;
    };

    template<typename Value>
    struct node_value_dependencies<Value,
                                   std::void_t<typename Value::required_t>> {
      using type = typename Value::required_t;
    };

    template<typename Value>
    using node_value_dependencies_t =
        typename node_value_dependencies<Value>::type;

    template<typename Value>
    struct has_node_value_dependencies
        : std::negation<
              std::is_same<node_value_dependencies_t<Value>, dependencies<>>> {
    };

    template<typename Value>
    inline static constexpr auto has_node_value_dependencies_v =
        has_node_value_dependencies<Value>::value;

    template<typename Population, typename Value>
    using node_value_dependencies_pack =
        dependencies_pack<Population, node_value_dependencies_t<Value>>;

    template<typename Population, typename Value>
    using node_value_dependencies_pack_t =
        typename dependencies_pack<Population, Value>::type;

    template<typename Population, typename Value, typename Source>
    auto pack_dependencies(Source const& source) {
      return dependencies_pack<Population, Value>::pack(Source);
    }

    template<typename Value, typename Population>
    struct is_node_value_constructor
        : std::conditional_t<
              has_node_value_dependencies_v<Value>,
              std::is_constructible<
                  typename Value::template type<Population>,
                  Population const&,
                  typename Value::template type<Population> const&,
                  node_value_dependencies_pack_t<Population, Value> const&>,
              std::is_constructible<
                  typename Value::template type<Population>,
                  Population const&,
                  typename Value::template type<Population> const&>> {};

    template<typename Value, typename Population>
    inline static constexpr auto is_node_value_constructor_v =
        is_node_value_constructor<Value, Population>::value;

  } // namespace details

  template<typename Value, typename Population>
  concept node_value =
      !std::is_final_v<typename Value::template type<Population>> &&
      details::is_node_value_constructor_v<Value, Population>;

  namespace details {

    template<typename Value>
    class node_value_constructor : public Value {
    public:
      template<typename Population>
      inline node_value_constructor(Population const& population,
                                    Value const& previous,
                                    std::tuple<> const& /*unused*/)
          : Value{population, previous} {
      }

      template<typename Population, typename Dependencies>
      inline node_value_constructor(Population const& population,
                                    Value const& previous,
                                    Dependencies const& dependencies)
          : Value{population, previous, dependencies} {
      }
    };

    template<typename Population, node_value<Population>... Values>
    class node;

    template<typename Population>
    class node<Population> {};

    template<typename Population, node_value<Population> Value>
    class node<Population, Value>
        : public node_value_constructor<
              typename Value::template type<Population>> {
    public:
      using constructor_t =
          node_value_constructor<typename Value::template type<Population>>;

    public:
      inline node(Population const& population,
                  node<Population> const& previous)
          : constructor_t{population,
                          previous,
                          pack_dependencies<Population, Value>(*this)} {
      }
    };

    template<typename Population,
             node_value<Population> Value,
             node_value<Population>... Rest>
    class node<Population, Value, Rest...>
        : public node_value_constructor<
              typename Value::template type<Population>>,
          public node<Population, Rest...> {
    public:
      using base_t = node<Population, Rest...>;
      using constructor_t =
          node_value_constructor<typename Value::template type<Population>>;

    public:
      inline node(Population const& population, node const& previous)
          : base_t{previous}
          , constructor_t{population,
                          previous,
                          pack_dependencies<Population, Value>(*this)} {
      }
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

  template<typename Tag>
  struct generic_counter {
    template<typename Population>
    class type {
    public:
      inline type(Population const& population, type const& previous) noexcept {
      }

      inline auto count_value() const noexcept {
        return count_;
      }

      inline void set_count_value(std::size_t count) noexcept {
        count_ = count;
      }

    private:
      std::size_t count_;
    };
  };

  template<typename Tag>
  struct generic_timer {
    template<typename Population>
    class type {
    public:
      using timer_t = std::chrono::high_resolution_clock;

    public:
      inline type(Population const& population, type const& previous) noexcept {
      }

      inline auto elapsed_value() const noexcept {
        return stop_ - start_;
      }

      inline void start_timer() noexcept {
        start_ = timer_t::now();
      }

      inline void stop_timer() noexcept {
        stop_ = timer_t::now();
      }

    private:
      timer_t::time_point start_;
      timer_t::time_point stop_;
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

      inline auto const& fitness_worst_value() const noexcept {
        return value_.first;
      }

      inline auto const& fitness_best_value() const noexcept {
        return value_.second;
      }

    private:
      minmax_fitness_t value_{};
    };
  };

  template<typename FitnessTag>
  struct total_fitness {
    template<averageable_population<FitnessTag> Population>
    class type {
    private:
      using fitness_tag_t = FitnessTag;
      using fitness_t = get_fitness_t<FitnessTag, Population>;
      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

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
                       .sum()} {
      }

      inline auto const& fitness_total_value() const noexcept {
        return value_;
      }

    private:
      fitness_t value_;
    };
  };

  template<typename FitnessTag>
  struct average_fitness {
    using fitness_tag_t = FitnessTag;
    using total_fitness_t = total_fitness<fitness_tag_t>;

    using required_t = dependencies<total_fitness_t>;

    template<averageable_population<FitnessTag> Population>
    class type {
    public:
      using pack_t = dependencies_pack<Population, required_t>;
      using dependencies_t = typename pack_t::type;

    private:
      using fitness_t = get_fitness_t<FitnessTag, Population>;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      inline type(Population const& population, type const& previous) noexcept {
        auto const& sum =
            unpack_dependency<pack_t, total_fitness_t>(dependencies)
                .fitness_total_value();

        value_ = sum / population.current_size();
      }

      inline auto const& fitness_average_value() const noexcept {
        return value_;
      }

    private:
      fitness_t value_;
    };
  };

  template<typename Value>
  struct square_power {
    auto operator()(Value const& value) const {
      return std::pow(value, 2);
    }
  };

  template<typename Value>
  using square_power_result_t =
      std::decay_t<std::invoke_result_t<square_power<Value>, Value>>;

  template<typename Value>
  struct square_root {
    auto operator()(Value const& value) const {
      return std::sqrt(value);
    }
  };

  template<typename Value>
  using square_root_result_t =
      std::decay_t<std::invoke_result_t<square_root<Value>, Value>>;

  template<typename FitnessTag>
  struct fitness_deviation {
    using fitness_tag_t = FitnessTag;
    using average_fitness_t = average_fitness<fitness_tag_t>;

    using required_t = dependencies<average_fitness_t>;

    template<averageable_population<fitness_tag_t> Population>
    class type {
    public:
      using pack_t = dependencies_pack<Population, required_t>;
      using dependencies_t = typename pack_t::type;

    private:
      using fitness_t = get_fitness_t<FitnessTag, Population>;

      using variance_t = square_power_result_t<fitness_t>;
      using deviation_t = square_root_result_t<variance_t>;

      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      inline type(Population const& population,
                  type const& previous,
                  dependencies_t const& dependencies) noexcept {
        auto const& avg =
            unpack_dependency<pack_t, average_fitness_t>(dependencies)
                .fitness_average_value();

        variance_ =
            std::accumulate(
                population.individuals().begin(),
                population.individuals().end(),
                state_t{},
                [](state_t acc, auto const& ind) {
                  return acc.add(pow_(avg - ind.evaluation().get(fitness_tag)));
                })
                .sum();

        deviation_ = sqrt_(variance_ / population.current_size());
      }

      inline auto const& fitness_variance_value() const noexcept {
        return variance_;
      }

      inline auto const& fitness_deviation_value() const noexcept {
        return deviation_;
      }

    private:
      [[no_unique_address]] square_power<fitness_t> pow_{};
      [[no_unique_address]] square_root<variance_t> sqrt_{};

      variance_t variance_;
      deviation_t deviation_;
    };
  };

  template<typename Population, node_value<Population>... Values>
  class statistics : public details::node<Population, Values...> {
  public:
    using population_t = Population;

  private:
    using base_t = details::node<population_t, Values...>;

    template<node_value<population_t> Value>
    using base_value_t = typename Value::template type<population_t>;

  private:
    inline statistics(population_t const& population,
                      statistics const& previous)
        : base_t{population, previous} {
    }

  public:
    inline auto next(population_t const& population) const {
      return statistics(population, *this);
    }

    template<node_value<population_t> Value>
    inline decltype(auto) get() noexcept {
      return static_cast<base_value_t<Value>&>(*this);
    }

    template<node_value<population_t> Value>
    inline decltype(auto) get() const noexcept {
      return static_cast<base_value_t<Value> const&>(*this);
    }
  };

  template<typename Statistics, typename Value>
  struct tracks_statistic
      : std::is_base_of<
            typename Value::template type<typename Statistics::population_t>,
            Statistics> {};

  template<typename Statistics, typename Value>
  inline constexpr auto tracks_statistic_v =
      tracks_statistic<Statistics, Value>::value;

  template<typename Statistics>
  concept statistical = requires(Statistics s) {
    typename Statistics::population_t;

    {
      s.next(std::declval<typename Statistics::population_t&>())
      } -> std::convertible_to<Statistics>;
  };

  template<statistical Statistics>
  class history {
  public:
    using statistics_t = Statistics;
    using population_t = typename statistics_t::population_t;

  public:
    inline explicit history(std::size_t depth)
        : depth_{depth} {
      values_.push(statistics_t{});
    }

    inline auto const& next(population_t const& population) {
      if (values_.size() >= depth_) {
        values_.pop();
      }

      return values_.push(current().next(population));
    }

    inline auto& current() noexcept {
      return values_.back();
    }

    inline auto const& current() const noexcept {
      return values_.back();
    }

  private:
    std::size_t depth_;
    std::queue<statistics_t> values_;
  };

} // namespace stats
} // namespace gal
