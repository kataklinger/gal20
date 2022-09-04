#pragma once

#include "configuration.hpp"

#include <functional>
#include <stop_token>

namespace gal {
namespace alg {

  namespace details {

    template<typename Builder>
    struct basic_scaling_cond
        : is_empty_fitness<typename Builder::scaled_fitness_t> {};

  } // namespace details

  using basic_config_map = config::entry_map<
      config::entry<config::root_ptype,
                    config::plist<config::size_ptype,
                                  config::init_ptype,
                                  config::tags_ptype>>,
      config::entry<
          config::init_ptype,
          config::plist<config::evaluate_ptype, config::reproduce_ptype>>,
      config::entry<config::evaluate_ptype,
                    config::plist<config::scale_fitness_ptype>>,
      config::entry<config::scale_fitness_ptype,
                    config::plist<config::statistics_ptype>>,
      config::entry<
          config::statistics_ptype,
          config::entry_if<
              details::basic_scaling_cond,
              config::plist<config::select_ptype, config::criterion_ptype>,
              config::plist<config::scale_ptype, config::criterion_ptype>>,
          config::plist<config::tags_ptype>>,
      config::entry<config::scale_ptype, config::plist<config::select_ptype>>,
      config::entry<config::select_ptype, config::plist<config::couple_ptype>>,
      config::entry<config::couple_ptype,
                    config::plist<config::replace_ptype>>>;

  template<typename Config>
  concept scaling_config =
      is_empty_fitness_v<typename Config::scaled_fitness_t> ||
      requires(Config c) {
    scaling<typename Config::scaling_t, typename Config::population_t>;
    {
      c.scaling(std::declval<typename Config::population_context_t>())
      } -> std::convertible_to<typename Config::scaling_t>;
  };

  template<typename Config>
  concept basic_config = scaling_config<Config> && requires(Config c) {
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

    template<basic_config Config>
    class local_scaler {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;
      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit local_scaler(config_t& config,
                                   population_context_t& context)
          : reproduction_{context.population(),
                          context.statistics(),
                          config.crossover(),
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

    template<basic_config Config>
    class global_scaler_base {
    public:
      using config_t = Config;

      using population_context_t = typename config_t::population_context_t;
      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit global_scaler_base(config_t& config,
                                         population_context_t& context)
          : reproduction_{context.population(),
                          context.statistics(),
                          config.crossover(),
                          config.mutation(),
                          config.evaluator()} {
      }

      inline auto reproduction() const {
        return reproduction_;
      }

    private:
      reproduction_context_t reproduction_;
    };

    template<basic_config Config>
    class global_scaler : public global_scaler_base<Config> {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;
      using scaling_t = typename config_t::scaling_t;

    public:
      inline explicit global_scaler(config_t& config,
                                    population_context_t& context)
          : global_scaler_base<config_t>{config, context}
          , context_{context}
          , scaling_{config.scaling(context)} {
      }

      inline void operator()() const noexcept {
        if constexpr (std::is_invocable_v<scaling_t>) {
          std::invoke(scaling_);
        }

        std::size_t rank{0};
        for (auto& individual : context_->population().individuals()) {
          std::invoke(scaling_, rank, individual);
        }
      }

    private:
      population_context_t* context_;
      scaling_t scaling_;
    };

    template<basic_config Config>
    class disabled_scaler : public global_scaler_base<Config> {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;

    public:
      inline explicit disabled_scaler(config_t& config,
                                      population_context_t& context)
          : global_scaler_base<config_t>{config, context} {
      }

      inline void operator()() const noexcept {
      }
    };

    template<basic_config Config>
    using scaler_t = std::conditional_t<
        is_empty_fitness_v<typename Config::scaled_fitness_t>,
        disabled_scaler<Config>,
        std::conditional_t<Config::is_global_scaling_t::value,
                           local_scaler<Config>,
                           global_scaler<Config>>>;

  } // namespace details

  template<basic_config Config>
  class basic {
  public:
    using config_t = Config;
    using population_t = typename config_t::population_t;
    using statistics_t = typename config_t::statistics_t;

  private:
    using individual_t = typename population_t::individual_t;
    using evaluation_t = typename individual_t::evaluation_t;
    using scaler_t = details::scaler_t<config_t>;

    using reproduction_context_t = typename config_t::reproduction_context_t;

  public:
    inline explicit basic(config_t const& config)
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
        stat::count_range(statistics, selection_count_tag, selected);

        auto offspring = couple(selected, coupling, statistics);
        stat::count_range(statistics, coupling_count_tag, offspring);

        auto replaced = replace(offspring, statistics);
        stat::count_range(statistics, selection_count_tag, replaced);
      }
    }

  private:
    inline void scale(scaler_t& scaler, statistics_t& current) {
      auto timer = stat::start_timer(current, scaling_time_tag);
      scaler();
    }

    inline auto select(statistics_t& current) {
      auto timer = stat::start_timer(current, selection_time_tag);
      return std::invoke(config_.selection(), population_);
    }

    template<std::ranges::range Selected,
             coupling<population_t, Selected> Coupling>
    inline auto couple(Selected&& selected,
                       Coupling&& coupling,
                       statistics_t& current) {
      auto coupling_time = stat::start_timer(current, coupling_time_tag);
      return std::invoke(std::forward<Coupling>(coupling),
                         std::forward<Selected>(selected));
    }

    template<std::ranges::range Offspring>
    inline auto replace(Offspring&& offspring, statistics_t& current) {
      auto timer = stat::start_timer(current, replacement_time_tag);
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
    stat::history<statistics_t> statistics_;
  };

} // namespace alg
} // namespace gal
