#pragma once

#include "config.hpp"

#include <functional>
#include <stop_token>

namespace gal {
namespace simple {

  namespace details {

    template<typename Builder>
    struct basic_algorithm_scaling_cond
        : is_empty_fitness<typename Builder::scaled_fitness_t> {};

  } // namespace details

  using basic_algorithm_config = config::entry_map<
      config::entry<config::root_iface,
                    config::iflist<config::size_iface,
                                   config::init_iface,
                                   config::tags_iface>>,
      config::entry<
          config::init_iface,
          config::iflist<config::evaluate_iface, config::reproduce_iface>>,
      config::entry<config::evaluate_iface,
                    config::iflist<config::scale_fitness_iface>>,
      config::entry<config::scale_fitness_iface,
                    config::iflist<config::statistics_iface>>,
      config::entry<
          config::statistics_iface,
          config::entry_if<
              details::basic_algorithm_scaling_cond,
              config::iflist<config::select_iface, config::criterion_iface>,
              config::iflist<config::scale_iface, config::criterion_iface>>,
          config::iflist<config::tags_iface>>,
      config::entry<config::scale_iface, config::iflist<config::select_iface>>,
      config::entry<config::select_iface, config::iflist<config::couple_iface>>,
      config::entry<config::couple_iface,
                    config::iflist<config::replace_iface>>>;

  template<typename Config>
  concept scaling_config =
      is_empty_fitness_v<typename Config::scaled_fitness_t> ||
      requires(Config c) {
    scaling<typename Config::scaling_t,
            typename Config::chromosome_t,
            typename Config::raw_fitness_t,
            typename Config::scaled_fitness_t>;
    {
      c.scaling(std::declval<typename Config::population_context_t>())
      } -> std::convertible_to<typename Config::scaling_t>;
  };

  template<typename Config>
  concept algorithm_config = scaling_config<Config> && requires(Config c) {
    chromosome<typename Config::chromosome_t>;
    fitness<typename Config::raw_fitness_t>;
    fitness<typename Config::scaled_fitness_t>;

    std::same_as<typename Config::population_t,
                 population<typename Config::chromosome_t,
                            typename Config::raw_fitness_t,
                            typename Config::scaled_fitness_t,
                            typename Config::tags_t>>;

    initializator<typename Config::initializator_t>;
    crossover<typename Config::crossover_t, typename Config::chromosome_t>;
    mutation<typename Config::mutation_t, typename Config::chromosome_t>;
    evaluator<typename Config::evaluator_t, typename Config::chromosome_t>;

    std::derived_from<typename Config::improving_mutation_t, std::true_type> ||
        std::derived_from<typename Config::improving_mutation_t,
                          std::false_type>;

    traits::boolean_flag<typename Config::is_global_scaling_t>;
    traits::boolean_flag<typename Config::is_stable_scaling_t>;

    selection_range<typename Config::selection_result_t,
                    typename Config::population_t::const_iterator_t>;
    replacement_range<typename Config::copuling_result_t,
                      typename Config::population_t::const_iterator_t,
                      typename Config::chromosome_t>;

    selection<typename Config::selection_t, typename Config::population_t>;
    coupling<typename Config::selection_t,
             typename Config::population_t,
             typename Config::selection_result_t>;
    replacement<typename Config::selection_t,
                typename Config::population_t,
                typename Config::copuling_result_t>;

    criterion<typename Config::criterion_t,
              typename Config::population_t,
              typename Config::statistics_t>;

    {
      c.initializator()
      } -> std::convertible_to<typename Config::initializator_t>;

    { c.evaluator() } -> std::convertible_to<typename Config::evaluator_t>;

    { c.selection() } -> std::convertible_to<typename Config::selection_t>;

    {
      c.coupling(std::declval<typename Config::reproduction_context_t>())
      } -> std::convertible_to<typename Config::coupling_t>;

    { c.criterion() } -> std::convertible_to<typename Config::criterion_t>;
  };

  namespace details {

    template<algorithm_config Config>
    class local_scaler {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;
      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit local_scaler(config_t& config,
                                   population_context_t& context)
          : reproduction_{config.crossover(),
                          config.mutation(),
                          config.evaluator(),
                          config.scaling(context)} {
      }

      inline void operator()() const noexcept {
      }

      inline auto reproduction() const {
        return reproduction_;
      }

    private:
      reproduction_context_t reproduction_;
    };

    template<algorithm_config Config>
    class global_scaler_base {
    public:
      using config_t = Config;

      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit global_scaler_base(config_t& config)
          : reproduction_{config.crossover(),
                          config.mutation(),
                          config.evaluator()} {
      }

      inline auto reproduction() const {
        return reproduction_;
      }

    private:
      reproduction_context_t reproduction_;
    };

    template<algorithm_config Config>
    class global_scaler : public global_scaler_base<Config> {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;
      using scaling_t = typename config_t::scaling_t;

    public:
      inline explicit global_scaler(config_t& config,
                                    population_context_t& context)
          : global_scaler_base<config_t>{config}
          , context_{context}
          , scaling_{config.scaling(context)} {
      }

      inline void operator()() const noexcept {
        if constexpr (std::is_invocable_v<scaling_t>) {
          std::invoke(scaling_);
        }

        for (auto& individual : context_->population().individuals()) {
          auto& evaluation = individual.evaluation();

          std::invoke(scaling_,
                      individual.chromosome(),
                      evaluation.raw(),
                      evaluation.scaled());
        }
      }

    private:
      population_context_t* context_;
      scaling_t scaling_;
    };

    template<algorithm_config Config>
    class disabled_scaler : public global_scaler_base<Config> {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;

    public:
      inline explicit disabled_scaler(config_t& config,
                                      population_context_t& /*unused*/)
          : global_scaler_base<config_t>{config} {
      }

      inline void operator()() const noexcept {
      }
    };

    template<algorithm_config Config>
    using scaler_t = std::conditional_t<
        is_empty_fitness_v<typename Config::scaled_fitness_t>,
        disabled_scaler<Config>,
        std::conditional_t<Config::is_global_scaling_t::value,
                           local_scaler<Config>,
                           global_scaler<Config>>>;

  } // namespace details

  template<algorithm_config Config>
  class algorithm {
  public:
    using config_t = Config;
    using population_t = typename config_t::population_t;
    using statistics_t = typename config_t::statistics_t;

  private:
    using individual_t = typename population_t::individual_t;
    using evaluation_t = typename individual_t::evaluation_t;
    using scaler_t = details::scaler_t<config_t>;

    using reproduction_context_t = typename config_t::reproduction_context_t;

    template<typename Tag>
    using tagged_t = stats::tagged_counter<Tag>;
    using input_t = coupling_metadata const&;

  public:
    inline explicit algorithm(config_t const& config)
        : config_{config}
        , population_{population_.population_size(),
                      config_t::is_stable_scaling_t::value} {
    }

    void run(std::stop_token token) {
      scaler_t scaler{config_};
      auto coupling = config_.coupling(scaler.reproduction());

      for (auto const* statistics = &init();
           !token.stop_requested() &&
           !config_.criterion(population_, *statistics);
           statistics = &statistics_.next(population_)) {

        scale(scaler, statistics);

        auto selected = select(statistics);
        stats::count_range<stats::selection_count_t>(statistics, selected);

        auto offspring = couple(selected, coupling, statistics);
        stats::count_range<stats::coupling_count_t>(statistics, offspring);
        stats::compute_composite(
            stats,
            offspring |
                std::views::transform([](auto const& item) -> decltype(auto) {
                  return get_metadata(item);
                }),

            [](input_t i, tagged_t<stats::corssover_count_t> s) {
              return decltype(s){s.value + i.crossover_performed};
            },

            [](input_t i, tagged_t<stats::mutation_tried_count_t> s) {
              return decltype(s){s.value + i.mutation_tried};
            },

            [](input_t i, tagged_t<stats::mutation_accepted_count_t> s) {
              return decltype(s){s.value + i.mutation_accepted};
            });

        auto replaced = replace(offspring, statistics);
        stats::count_range<stats::selection_count_t>(statistics, replaced);
      }
    }

  private:
    inline void scale(scaler_t& scaler, statistics_t& current) {
      auto timer = stats::start_timer<stats::scaling_time_t>(current);
      scaler();
    }

    inline auto select(statistics_t& current) {
      auto timer = stats::start_timer<stats::selection_time_t>(current);
      return std::invoke(config_.selection(), population_);
    }

    template<std::ranges::range Selected,
             coupling<population_t, Selected> Coupling>
    inline auto couple(Selected&& selected,
                       Coupling&& coupling,
                       statistics_t& current) {
      auto coupling_time = stats::start_timer<stats::coupling_time_t>(current);
      return std::invoke(std::forward<Coupling>(coupling),
                         std::forward<Selected>(selected));
    }

    template<std::ranges::range Offspring>
    inline auto replace(Offspring&& offspring, statistics_t& current) {
      auto timer = stats::start_timer<stats::replacement_time_t>(current);
      return std::invoke(config_.replacement(),
                         population_,
                         std::forward<Offspring>(offspring));
    }

  private:
    auto const& init() {
      std::ranges::generate_n(
          std::back_inserter(population_.individuals()),
          population_.target_size(),
          [&config_] {
            auto chromosome = config_.initializator()();

            evaluation_t evaluation{};
            std::invoke(config_.evaluator(), chromosome, evaluation.raw());

            return individual_t{std::move(chromosome), std::move(evaluation)};
          });

      return statistics_.next(population_);
    }

  private:
    config_t config_;
    population_t population_;
    stats::history<statistics_t> statistics_;
  };

} // namespace simple
} // namespace gal