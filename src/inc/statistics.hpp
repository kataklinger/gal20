
#pragma once

#include "operation.hpp"

#include <chrono>
#include <numeric>
#include <queue>
#include <tuple>

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
        std::add_const_t<typename Dependency::template body<Population>>>;

    using type = std::tuple<element_t<Dependencies>...>;

    template<typename Source>
    inline static auto pack(Source const& source) {
      return type{static_cast<element_t<Dependencies>>(source)...};
    }

    template<typename Dependency>
    inline static decltype(auto) unpack(type const& pack) {
      return std::get<element_t<Dependency>>(pack);
    }
  };

  template<typename Pack, typename Dependency, typename Dependencies>
  decltype(auto) unpack_dependency(Dependencies const& dependencies) {
    return Pack::template unpack<Dependency>(dependencies);
  }

  namespace details {

    template<typename Model, typename = void>
    struct get_model_dependencies : std::false_type {
      using list = dependencies<>;
    };

    template<typename Model>
    struct get_model_dependencies<Model,
                                  std::void_t<typename Model::required_t>>
        : std::true_type {
      using list = typename Model::required_t;
    };

    template<typename Model>
    using get_model_dependencies_t =
        typename get_model_dependencies<Model>::list;

    template<typename Model>
    struct has_model_dependencies : get_model_dependencies<Model> {};

    template<typename Model>
    inline static constexpr auto has_model_dependencies_v =
        has_model_dependencies<Model>::value;

    template<typename Population, typename Model>
    using model_dependencies_pack =
        dependencies_pack<Population, get_model_dependencies_t<Model>>;

    template<typename Population, typename Model>
    using model_dependencies_pack_t =
        typename dependencies_pack<Population,
                                   typename Model::required_t>::type;

    template<typename Population, typename Model, typename Source>
    inline auto pack_dependencies(Source const& source) {
      return dependencies_pack<Population,
                               get_model_dependencies_t<Model>>::pack(source);
    }

    template<typename Model, typename Population>
    struct is_constructible_with_dependencies {
      using type = std::is_constructible<
          typename Model::template body<Population>,
          Population const&,
          typename Model::template body<Population> const&,
          model_dependencies_pack_t<Population, Model> const&>;
    };

    template<typename Model, typename Population>
    struct is_constructible_without_dependencies {
      using type = std::is_constructible<
          typename Model::template body<Population>,
          Population const&,
          typename Model::template body<Population> const&>;
    };

    template<typename Model, typename Population>
    struct is_model_constructor
        : std::conditional_t<
              has_model_dependencies_v<Model>,
              is_constructible_with_dependencies<Model, Population>,
              is_constructible_without_dependencies<Model, Population>>::type {
    };

    template<typename Model, typename Population>
    inline static constexpr auto is_model_constructor_v =
        is_model_constructor<Model, Population>::value;

  } // namespace details

  // clang-format off

  template<typename Model, typename Population>
  concept model = std::semiregular<typename Model::template body<Population>> &&
                  !std::is_final_v<typename Model::template body<Population>> &&
                  details::is_model_constructor_v<Model, Population>;

  // clang-format on

  namespace details {

    template<typename Value>
    class model_constructor : public Value {
    public:
      model_constructor() = default;

      template<typename Population>
      inline model_constructor(Population const& population,
                               Value const& previous,
                               std::tuple<> const& /*unused*/)
          : Value{population, previous} {
      }

      template<typename Population, typename Dependencies>
      inline model_constructor(Population const& population,
                               Value const& previous,
                               Dependencies const& dependencies)
          : Value{population, previous, dependencies} {
      }
    };

    template<typename Population, model<Population>... Models>
    class model_node;

    template<typename Population>
    class model_node<Population> {};

    template<typename Population, model<Population> Model>
    class model_node<Population, Model>
        : public model_constructor<typename Model::template body<Population>> {
    public:
      using constructor_t =
          model_constructor<typename Model::template body<Population>>;

    public:
      model_node() = default;

      inline model_node(Population const& population,
                        model_node<Population> const& previous)
          : constructor_t{population,
                          previous,
                          pack_dependencies<Population, Model>(*this)} {
      }
    };

    template<typename Population,
             model<Population> Model,
             model<Population>... Rest>
    class model_node<Population, Model, Rest...>
        : public model_constructor<typename Model::template body<Population>>,
          public model_node<Population, Rest...> {
    public:
      using base_t = model_node<Population, Rest...>;
      using constructor_t =
          model_constructor<typename Model::template body<Population>>;

    public:
      model_node() = default;

      inline model_node(Population const& population,
                        model_node const& previous)
          : base_t{previous}
          , constructor_t{population,
                          previous,
                          pack_dependencies<Population, Model>(*this)} {
      }
    };

  } // namespace details

  struct generation {
    template<typename Population>
    class body {
    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept
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
    class body {
    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept
          : value_{population.current_size()} {
      }

      inline auto population_size_value() const noexcept {
        return value_;
      }

    private:
      std::size_t value_{};
    };
  };

  template<std::semiregular Value, typename Tag>
  struct generic_value {
    template<typename Population>
    class body {
    public:
      using value_t = Value;
      using tag_t = Tag;

    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept {
      }

      inline auto generic_value() const noexcept {
        return value_;
      }

      inline void set_generic_value(value_t value) noexcept {
        value_ = value;
      }

    private:
      value_t value_;
    };
  };

  template<typename Tag>
  using generic_counter = generic_value<std::size_t, Tag>;

  template<typename Tag>
  struct generic_timer {
    template<typename Population>
    class body {
    public:
      using tag_t = Tag;
      using timer_t = std::chrono::high_resolution_clock;

    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept {
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
    class body {
    private:
      using fitness_tag_t = FitnessTag;

      using fitness_t = get_fitness_t<FitnessTag, Population>;
      using minmax_fitness_t = std::pair<fitness_t, fitness_t>;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept {
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

  using extreme_fitness_raw = extreme_fitness<raw_fitness_tag>;
  using extreme_fitness_scaled = extreme_fitness<scaled_fitness_tag>;

  template<typename FitnessTag>
  struct total_fitness {
    template<averageable_population<FitnessTag> Population>
    class body {
    private:
      using fitness_tag_t = FitnessTag;
      using fitness_t = get_fitness_t<FitnessTag, Population>;
      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      body() = default;

      inline body(Population const& population, body const& previous) noexcept
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
      fitness_t value_{};
    };
  };

  using total_fitness_raw = total_fitness<raw_fitness_tag>;
  using total_fitness_scaled = total_fitness<scaled_fitness_tag>;

  template<typename FitnessTag>
  struct average_fitness {
    using fitness_tag_t = FitnessTag;
    using total_fitness_t = total_fitness<fitness_tag_t>;

    using required_t = dependencies<total_fitness_t>;

    template<averageable_population<FitnessTag> Population>
    class body {
    public:
      using pack_t = dependencies_pack<Population, required_t>;
      using dependencies_t = typename pack_t::type;

    private:
      using fitness_t = get_fitness_t<FitnessTag, Population>;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      body() = default;

      inline body(Population const& population,
                  body const& previous,
                  dependencies_t const& dependencies) noexcept {
        auto const& sum =
            unpack_dependency<pack_t, total_fitness_t>(dependencies)
                .fitness_total_value();

        value_ = sum / population.current_size();
      }

      inline auto const& fitness_average_value() const noexcept {
        return value_;
      }

    private:
      fitness_t value_{};
    };
  };

  using average_fitness_raw = average_fitness<raw_fitness_tag>;
  using average_fitness_scaled = average_fitness<scaled_fitness_tag>;

  template<typename Value>
  struct square_power;

  template<util::arithmetic Value>
  struct square_power<Value> {
    using result_t = Value;

    inline static constexpr Value power = 2;

    auto operator()(Value const& value) const {
      return std::pow(value, power);
    }
  };

  template<typename Value>
  using square_power_result_t = typename square_power<Value>::result_t;

  template<typename Value>
  struct square_root;

  template<util::arithmetic Value>
  struct square_root<Value> {
    using result_t = Value;

    auto operator()(Value const& value) const {
      return std::sqrt(value);
    }
  };

  template<typename Value>
  using square_root_result_t = typename square_root<Value>::result_t;

  template<typename Fitness>
  concept deviation_fitness =
      arithmetic_fitness<Fitness> && requires {
                                       typename square_root<Fitness>::result_t;
                                       typename square_root<Fitness>::result_t;
                                     };

  template<deviation_fitness Fitness>
  struct fitness_deviation_types {
    using variance_t = square_power_result_t<Fitness>;
    using deviation_t = square_root_result_t<variance_t>;
  };

  template<deviation_fitness Fitness>
  using fitness_variance_t =
      typename fitness_deviation_types<Fitness>::variance_t;

  template<deviation_fitness Fitness>
  using fitness_deviation_t = typename fitness_deviation_types<
      fitness_variance_t<Fitness>>::deviation_t;

  template<typename Population, typename FitnessTag>
  concept deviable_population =
      averageable_population<Population, FitnessTag> &&
      deviation_fitness<get_fitness_t<FitnessTag, Population>>;

  template<typename FitnessTag>
  struct fitness_deviation {
    using fitness_tag_t = FitnessTag;
    using average_fitness_t = average_fitness<fitness_tag_t>;

    using required_t = dependencies<average_fitness_t>;

    template<deviable_population<fitness_tag_t> Population>
    class body {
    public:
      using pack_t = dependencies_pack<Population, required_t>;
      using dependencies_t = typename pack_t::type;

    private:
      using fitness_t = get_fitness_t<FitnessTag, Population>;

      using variance_t = fitness_variance_t<fitness_t>;
      using deviation_t = fitness_deviation_t<fitness_t>;

      using state_t = typename fitness_traits<fitness_t>::totalizator_t;

      inline static constexpr fitness_tag_t fitness_tag{};

    public:
      body() = default;

      inline body(Population const& population,
                  body const& previous,
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

  using fitness_deviation_raw = fitness_deviation<raw_fitness_tag>;
  using fitness_deviation_scaled = fitness_deviation<scaled_fitness_tag>;

  template<typename Population, model<Population>... Models>
  class statistics : public details::model_node<Population, Models...> {
  public:
    using population_t = Population;

  private:
    using base_t = details::model_node<population_t, Models...>;

    template<model<population_t> Model>
    using base_value_t = typename Model::template body<population_t>;

  private:
    inline statistics(population_t const& population,
                      statistics const& previous)
        : base_t{population, previous} {
    }

  public:
    inline statistics() {
    }

    inline auto next(population_t const& population) const {
      return statistics(population, *this);
    }

    template<model<population_t> Model>
    inline decltype(auto) get() noexcept {
      return static_cast<base_value_t<Model>&>(*this);
    }

    template<model<population_t> Model>
    inline decltype(auto) get() const noexcept {
      return static_cast<base_value_t<Model> const&>(*this);
    }
  };

  template<typename Statistics, typename Model>
  struct tracks_model
      : std::is_base_of<
            typename Model::template body<typename Statistics::population_t>,
            Statistics> {};

  template<typename Statistics, typename Model>
  inline constexpr auto tracks_model_v = tracks_model<Statistics, Model>::value;

  template<typename Statistics, typename... Models>
  struct tracks_models : std::conjunction<tracks_model<Statistics, Models>...> {
  };

  template<typename Statistics, typename... Models>
  inline constexpr auto tracks_models_v =
      tracks_models<Statistics, Models...>::value;

  template<typename Statistics, typename... Models>
  concept tracked_models = tracks_models_v<Statistics, Models...>;

  template<typename Statistics>
  concept statistical =
      requires(Statistics s) {
        typename Statistics::population_t;

        {
          s.next(std::declval<typename Statistics::population_t&>())
          } -> std::convertible_to<Statistics>;
      };

  struct disabled_timer {};

  template<typename Statistics, model<typename Statistics::population_t> Timer>
  class enabled_timer {
  private:
    using statistics_t = Statistics;
    using timer_t = Timer;

    using population_t = typename statistics_t::population_t;
    using value_t = typename timer_t::template body<population_t>;

  public:
    inline explicit enabled_timer(Statistics& statistics)
        : value_{&statistics.get<timer_t>()} {
      value_->start_timer();
    }

    inline ~enabled_timer() noexcept {
      value_->stop_timer();
    }

    enabled_timer(enabled_timer&&) = delete;
    enabled_timer(enabled_timer const&) = delete;

    inline enabled_timer& operator=(enabled_timer&&) = delete;
    enabled_timer& operator=(enabled_timer const&) = delete;

  private:
    value_t* value_;
  };

  template<typename Tag, typename Statistics>
  inline auto start_timer(Statistics& statistics, Tag /*unused*/) {
    using timer_t = generic_timer<Tag>;

    if constexpr (tracks_model_v<Statistics, timer_t>) {
      return enabled_timer<Statistics, timer_t>{statistics};
    }
    else {
      return disabled_timer{};
    }
  }

  template<typename Tag, typename Statistics, std::ranges::sized_range Range>
  inline void
      count_range(Statistics& statistics, Tag /*unused*/, Range const& range) {
    using counter_t = generic_counter<Tag>;

    if constexpr (tracks_model_v<Statistics, counter_t>) {
      statistics.get<counter_t>().set_generic_value(std::ranges::size(range));
    }
  }

  template<typename Tag, typename Statistics>
  inline void increment_count(Statistics& statistics,
                              Tag /*unused*/,
                              std::size_t increment = 1) {
    using counter_t = generic_counter<Tag>;

    if constexpr (tracks_model_v<Statistics, counter_t>) {
      auto& model = statistics.get<counter_t>();
      model.set_generic_value(model.get_generic_value() + increment);
    }
  }

  namespace details {
    template<typename Fn>
    using compute_result_t = std::remove_cvref_t<std::invoke_result_t<Fn>>;
  }

  // clang-format off

  template<typename Fn>
  concept simple_computation = std::is_invocable_v<Fn> &&
      !std::is_same_v<details::compute_result_t<Fn>, void>;

  // clang-format on

  template<typename Tag, typename Statistics, simple_computation Fn>
  inline void compute_simple(Statistics& statistics, Tag /*unused*/, Fn&& fn) {
    using value_t = generic_value<details::compute_result_t<Fn>, Tag>;

    if constexpr (tracks_model_v<Statistics, value_t>) {
      statistics.get<value_t>().set_generic_value(
          std::invoke(std::forward<Fn>(fn)));
    }
  }

  template<std::semiregular Value, typename Tag>
  struct tagged {
    using tag_t = Tag;
    using value_t = Value;

    value_t value;
  };

  template<typename Tag>
  using tagged_counter = tagged<std::size_t, Tag>;

  namespace details {

    template<typename Fn, typename Input, typename Value>
    using is_matching_value = std::is_invocable_r<Value, Fn, Input, Value>;

    template<typename Fn, typename Input, typename Value>
    inline constexpr auto is_matching_value_v =
        is_matching_value<Fn, Input, Value>::value;

    template<typename... Values>
    struct vlist {};

    template<typename Added, typename List>
    struct add_vlist;

    template<typename Added, typename... Values>
    struct add_vlist<Added, vlist<Values...>> {
      using type = vlist<Added, Values...>;
    };

    template<typename Added, typename List>
    using add_vlist_t = add_vlist<Added, List>;

    template<typename Fn, typename Input, typename Values>
    struct find_matching_value;

    template<typename Fn, typename Input>
    struct find_matching_value<Fn, Input, vlist<>> : std::false_type {
      using value_t = void;
    };

    template<typename Value>
    struct matched_value : std::true_type {
      using value_t = Value;
    };

    template<typename Fn, typename Input, typename Value, typename... Rest>
    struct find_matching_value<Fn, Input, vlist<Value, Rest...>>
        : std::conditional_t<is_matching_value_v<Fn, Input, Value>,
                             matched_value<Value>,
                             find_matching_value<Fn, Input, vlist<Rest...>>> {};

    struct nongeneric_value {};

    template<typename Value, typename = void>
    struct get_tagged_value_helper {
      using type = nongeneric_value;
    };

    template<typename Value>
    struct get_tagged_value_helper<
        Value,
        std::void_t<typename Value::value_t, typename Value::tag_t>> {
      using type = tagged<typename Value::value_t, typename Value::tag_t>;
    };

    template<typename Value>
    using get_tagged_value_helper_t =
        typename get_tagged_value_helper<Value>::type;

    template<typename Population, model<Population> Model>
    using get_tagged_value_t =
        get_tagged_value_helper_t<typename Model::template body<Population>>;

    template<typename List>
    struct clean_vlist;

    template<>
    struct clean_vlist<vlist<>> {
      using type = vlist<>;
    };

    template<typename Value, typename... Rest>
    struct clean_vlist<vlist<Value, Rest...>> {
      using type = add_vlist_t<Value, typename clean_vlist<Rest...>::type>;
    };

    template<typename... Rest>
    struct clean_vlist<vlist<nongeneric_value, Rest...>>
        : clean_vlist<Rest...> {};

    template<typename List>
    using clean_vlist_t = typename clean_vlist<List>::type;

    template<typename Population, model<Population>... Models>
    struct convert_to_vlist {
      using type =
          clean_vlist_t<vlist<get_tagged_value_t<Population, Models>...>>;
    };

    template<typename Population, model<Population>... Models>
    using convert_to_vlist_t =
        typename convert_to_vlist<Population, Models...>::type;

    template<typename Input, typename Values, typename... Fns>
    struct composite_state_helper;

    template<typename Input, typename Values>
    struct composite_state_helper<Input, Values> {
      using type = vlist<>;
    };

    template<typename Input, typename Values, typename Fn, typename... Rest>
    struct composite_state_helper<Input, Values, Fn, Rest...> {
      using match_t = find_matching_value<Fn, Input, Values>;
      using rest_t =
          typename composite_state_helper<Input, Values, Rest...>::type;

      using type =
          std::conditional_t<match_t::value,
                             add_vlist_t<typename match_t::value_t, rest_t>,
                             rest_t>;
    };

    template<typename List>
    struct vlist_to_tuple;

    template<typename... Values>
    struct vlist_to_tuple<vlist<Values...>> {
      using type = std::tuple<Values...>;
    };

    template<typename List>
    using vlist_to_tuple_t = typename vlist_to_tuple<List>::type;

    template<typename Tuple>
    struct tuple_to_vlist;

    template<typename... Values>
    struct tuple_to_vlist<std::tuple<Values...>> {
      using type = vlist<Values...>;
    };

    template<typename Tuple>
    using tuple_to_vlist_t = typename tuple_to_vlist<Tuple>::type;

    template<typename Input, typename Values, typename... Fns>
    struct composite_state {
      using type =
          vlist_to_tuple_t<composite_state_helper<Input, Values, Fns...>>;
    };

    template<typename Input, typename Values, typename... Fns>
    using composite_state_t =
        typename composite_state<Input, Values, Fns...>::type;

    struct skipped_value {};

    template<typename Fn, typename State, typename Input>
    auto compute_single(Fn&& fn, State state, Input const& input) {
      using match_t = find_matching_value<Fn, Input, tuple_to_vlist_t<State>>;
      if constexpr (match_t::value) {
        return std::invoke(std::forward<Fn>(fn),
                           input,
                           std::get<typename match_t::value_t>(state));
      }
      else {
        return skipped_value{};
      }
    }

    template<std::size_t Idx, typename Idxs>
    struct add_index_sequence;

    template<std::size_t Idx, std::size_t... Idxs>
    struct add_index_sequence<Idx, std::index_sequence<Idxs...>> {
      using type = std::index_sequence<Idx, Idxs...>;
    };

    template<std::size_t Idx, typename Idxs>
    using add_index_sequence_t = typename add_index_sequence<Idx, Idxs>::type;

    template<typename Idxs, typename Values>
    struct filter_components;

    template<>
    struct filter_components<std::index_sequence<>, vlist<>> {
      using type = std::index_sequence<>;
    };

    template<std::size_t Idx,
             std::size_t... Idxs,
             typename Value,
             typename... Values>
    struct filter_components<std::index_sequence<Idx, Idxs...>,
                             vlist<Value, Values...>> {
      using rest_t = typename filter_components<std::index_sequence<Idxs...>,
                                                vlist<Values...>>::type;
      using type = add_index_sequence_t<Idx, rest_t>;
    };

    template<std::size_t Idx, std::size_t... Idxs, typename... Values>
    struct filter_components<std::index_sequence<Idx, Idxs...>,
                             vlist<skipped_value, Values...>> {
      using type = typename filter_components<std::index_sequence<Idxs...>,
                                              vlist<Values...>>::type;
    };

    template<typename Idxs, typename Values>
    using filter_components_t = typename filter_components<Idxs, Values>::type;

    template<typename... Tys, std::size_t... Idxs>
    auto pack_helper(std::tuple<Tys...>&& raw,
                     std::index_sequence<Idxs...> /*unused*/) noexcept {
      return std::tuple{std::move(std::get<Idxs>(raw))...};
    }

    template<typename... Tys>
    auto pack_composite(std::tuple<Tys...>&& raw) noexcept {
      using indices_t =
          filter_components_t<std::index_sequence_for<Tys...>, vlist<Tys...>>;
      return pack_helper(std::move(raw), indices_t{});
    }

    template<typename Statistics, typename Value, typename Tag>
    inline void unpack_single(Statistics& statistics,
                              tagged<Value, Tag>&& single) {
      using value_t = generic_value<Value, Tag>;

      statistics.get<value_t>().set_generic_value(std::move(single.value));
    }

    template<typename Statistics, typename... Tys>
    inline void unpack_composite(Statistics& statistics,
                                 std::tuple<Tys...>&& pack) {
      (unpack_single(statistics, std::move(std::get<Tys>(pack))), ...);
    }

  } // namespace details

  template<typename Population,
           model<Population>... Models,
           std::ranges::range Range,
           typename... Fns>
  inline auto compute_composite(statistics<Population, Models...>& statistics,
                                Range const& range,
                                Fns&&... fns) {
    using input_t = std::remove_cvref_t<decltype(*std::ranges::cbegin(range))>;
    using vlist_t = details::convert_to_vlist_t<Population, Models...>;
    using state_t = details::composite_state_t<input_t, vlist_t, Fns...>;

    if constexpr (!std::is_same_v<std::tuple<>, state_t>) {
      auto result = std::accumulate(
          std::ranges::cbegin(range),
          std::ranges::cend(range),
          state_t{},
          [&fns...](state_t s, input_t const& i) {
            std::tuple components{details::compute_single(fns, s, i)...};
            return details::pack_composite(std::move(components));
          });

      details::unpack_composite(statistics, std::move(result));
    }
  }

  namespace details {

    template<typename Result>
    struct is_complex_result : std::false_type {};

    template<typename... Values, typename... Tags>
    struct is_complex_result<std::tuple<tagged<Values, Tags>...>>
        : std::true_type {};

    template<typename Result>
    inline constexpr auto is_complex_result_v =
        is_complex_result<Result>::value;

    template<typename Statistics, typename Idxs, typename Values>
    struct filter_complex_result;

    template<typename Statistics,
             std::size_t Idx,
             std::size_t... Idxs,
             typename Value,
             typename Tag,
             typename... Values>
    struct filter_complex_result<Statistics,
                                 std::index_sequence<Idx, Idxs...>,
                                 std::tuple<tagged<Value, Tag>, Values...>> {
      using rest_t = filter_complex_result<Statistics,
                                           std::index_sequence<Idxs...>,
                                           std::tuple<Values...>>;
      using type = std::conditional_t<
          tracks_model_v<Statistics, generic_value<Value, Tag>>,
          add_index_sequence_t<Idx, rest_t>,
          rest_t>;
    };

    template<typename Statistics, typename Idxs, typename Values>
    using filter_complex_result_t =
        typename filter_complex_result<Statistics, Idxs, Values>::type;

  } // namespace details

  template<typename Fn>
  concept complex_computation =
      std::is_invocable_v<Fn> &&
      details::is_complex_result_v<details::compute_result_t<Fn>>;

  template<typename Statistics, complex_computation Fn>
  inline void compute_complex(Statistics& statistics, Fn&& fn) {
    using result_t = details::compute_result_t<Fn>;
    using sequence_t = std::make_index_sequence<std::tuple_size_v<result_t>>;
    using indices_t =
        details::filter_complex_result_t<Statistics, sequence_t, result_t>;

    if constexpr (!std::is_same_v<indices_t, std::index_sequence<>>) {
      details::unpack_composite(
          statistics,
          details::pack_helper(std::invoke(std::forward<Fn>(fn)), indices_t{}));
    }
  }

  struct get_generation {
    using model_t = generation;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().generation_value();
    }
  };

  struct get_population_size {
    using model_t = population_size;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().population_size_value();
    }
  };

  template<std::semiregular Value, typename Tag>
  struct get_generic_value {
    using model_t = generic_value<Value, Tag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().generic_value();
    }
  };

  template<typename Tag>
  struct get_timer_value {
    using model_t = generic_timer<Tag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().elapsed_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_worst_value {
    using model_t = extreme_fitness<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_worst_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_best_value {
    using model_t = extreme_fitness<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_best_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_total_value {
    using model_t = total_fitness<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_total_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_average_value {
    using model_t = average_fitness<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_average_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_variance_value {
    using model_t = fitness_deviation<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_variance_value();
    }
  };

  template<typename FitnessTag>
  struct get_fitness_deviation_value {
    using model_t = fitness_deviation<FitnessTag>;

    template<tracked_models<model_t> Statistics>
    inline decltype(auto)
        operator()(Statistics const& statistics) const noexcept {
      return statistics.template get<model_t>().fitness_deviation_value();
    }
  };

  template<statistical Statistics>
  class history {
  public:
    using statistics_t = Statistics;
    using population_t = typename statistics_t::population_t;

  public:
    inline explicit history(std::size_t depth)
        : depth_{depth} {
      values_.push_back(statistics_t{});
    }

    inline auto& next(population_t const& population) {
      if (values_.size() >= depth_) {
        values_.pop_front();
      }

      return values_.emplace_back(current().next(population));
    }

    inline auto& current() noexcept {
      return values_.back();
    }

    inline auto const& current() const noexcept {
      return values_.back();
    }

    inline auto& previous() noexcept {
      return *(values_.rbegin() + 1);
    }

    inline auto const& previous() const noexcept {
      return *(values_.rbegin() + 1);
    }

  private:
    std::size_t depth_;
    std::deque<statistics_t> values_;
  };

} // namespace stats
} // namespace gal
