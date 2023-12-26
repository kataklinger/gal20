#pragma once

#include "algorithm.hpp"

#include <functional>
#include <stop_token>

namespace gal {
namespace soo {

  namespace details {

    template<typename Builder>
    struct algo_scaling_cond
        : is_empty_fitness<typename Builder::scaled_fitness_t> {};

  } // namespace details

  struct algo_config_map {
    using type = config::entry_map<

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

        config::entry<config::statistics_ptype,
                      config::entry_if<details::algo_scaling_cond,
                                       config::plist<config::select_ptype,
                                                     config::criterion_ptype,
                                                     config::observe_ptype>,
                                       config::plist<config::scale_ptype,
                                                     config::criterion_ptype,
                                                     config::observe_ptype>>,
                      config::plist<config::tags_ptype>>,

        config::entry<config::scale_ptype, config::plist<config::select_ptype>>,

        config::entry<config::select_ptype,
                      config::plist<config::couple_ptype>>,

        config::entry<config::couple_ptype,
                      config::plist<config::replace_ptype>>>;
  };

  template<typename Config>
  concept scaling_config =
      is_empty_fitness_v<typename Config::scaled_fitness_t> ||
      requires(Config c) {
        requires scaling<typename Config::scaling_t,
                         typename Config::population_t>;

        {
          c.scaling(std::declval<typename Config::population_context_t&>())
        } -> std::convertible_to<typename Config::scaling_t>;
      };

  template<typename Config>
  concept algo_config =
      scaling_config<Config> && basic_algo_config<Config> && requires {
        requires util::boolean_flag<typename Config::is_global_scaling_t>;
        requires util::boolean_flag<typename Config::is_stable_scaling_t>;
      };

  namespace details {

    template<algo_config Config>
    class local_scaler {
    public:
      using config_t = Config;
      using population_context_t = typename config_t::population_context_t;
      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit local_scaler(config_t& config,
                                   population_context_t& context)
          : reproduction_{context.population(),
                          context.history(),
                          config.crossover(),
                          config.mutation(),
                          config.evaluator(),
                          config.scaling(context)} {
      }

      inline void operator()() const noexcept {
      }

      inline auto& reproduction() {
        return reproduction_;
      }

    private:
      reproduction_context_t reproduction_;
    };

    template<algo_config Config>
    class global_scaler_base {
    public:
      using config_t = Config;

      using population_context_t = typename config_t::population_context_t;
      using reproduction_context_t = typename config_t::reproduction_context_t;

    public:
      inline explicit global_scaler_base(config_t& config,
                                         population_context_t& context)
          : reproduction_{context.population(),
                          context.history(),
                          config.crossover(),
                          config.mutation(),
                          config.evaluator()} {
      }

      inline auto& reproduction() {
        return reproduction_;
      }

    private:
      reproduction_context_t reproduction_;
    };

    template<algo_config Config>
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

        std::size_t ordinal{0};
        for (auto&& individual : context_->population().individuals()) {
          std::invoke(scaling_, ordinal, individual);
        }
      }

    private:
      population_context_t* context_;
      scaling_t scaling_;
    };

    template<algo_config Config>
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

    template<algo_config Config>
    using scaler_t = std::conditional_t<
        is_empty_fitness_v<typename Config::scaled_fitness_t>,
        disabled_scaler<Config>,
        std::conditional_t<Config::is_global_scaling_t::value,
                           local_scaler<Config>,
                           global_scaler<Config>>>;

  } // namespace details

  template<algo_config Config>
  class algo {
  public:
    using config_t = Config;
    using population_t = typename config_t::population_t;
    using statistics_t = typename config_t::statistics_t;

  private:
    using individual_t = typename population_t::individual_t;
    using evaluation_t = typename individual_t::evaluation_t;
    using scaler_t = details::scaler_t<config_t>;

    using population_context_t = typename config_t::population_context_t;
    using reproduction_context_t = typename config_t::reproduction_context_t;

  public:
    inline explicit algo(config_t const& config)
        : config_{config}
        , population_{config_.raw_comparator(),
                      config_.scaled_comparator(),
                      config_.population_size(),
                      config_t::is_stable_scaling_t::value}
        , statistics_{config.statistics_depth()} {
    }

    void run(std::stop_token token) {
      population_context_t ctx{population_, statistics_};

      scaler_t scaler{config_, ctx};
      auto coupler = config_.coupling(scaler.reproduction());

      auto* statistics = &init();
      while (!token.stop_requested() &&
             !std::invoke(config_.criterion(), population_, statistics_)) {

        scale(scaler, *statistics);

        auto selected = select(*statistics);
        stats::count_range(*statistics, selection_count_tag, selected);

        auto offspring = couple(selected, coupler, *statistics);
        stats::count_range(*statistics, coupling_count_tag, offspring);

        auto replaced = replace(offspring, *statistics);
        stats::count_range(*statistics, selection_count_tag, replaced);

        statistics = &statistics_.next(population_);

        config_.observers().observe(generation_event, population_, statistics_);
      }
    }

  private:
    inline void scale(scaler_t& scaler, statistics_t& current) {
      auto timer = stats::start_timer(current, scaling_time_tag);
      scaler();
    }

    inline auto select(statistics_t& current) {
      auto timer = stats::start_timer(current, selection_time_tag);
      return std::invoke(config_.selection(), population_);
    }

    template<std::ranges::range Selected,
             coupling<population_t, Selected> Coupling>
    inline auto couple(Selected&& selected,
                       Coupling&& coupling,
                       statistics_t& current) {
      auto coupling_time = stats::start_timer(current, coupling_time_tag);
      return std::invoke(std::forward<Coupling>(coupling),
                         std::forward<Selected>(selected));
    }

    template<std::ranges::range Offspring>
    inline auto replace(Offspring&& offspring, statistics_t& current) {
      auto timer = stats::start_timer(current, replacement_time_tag);
      return std::invoke(config_.replacement(),
                         population_,
                         std::forward<Offspring>(offspring));
    }

    auto& init() {
      std::ranges::generate_n(
          std::back_inserter(population_.individuals()),
          config_.population_size(),
          [this] {
            auto chromo = config_.initializator()();

            return individual_t{
                std::move(chromo),
                evaluation_t{std::invoke(config_.evaluator(), chromo)}};
          });

      return statistics_.next(population_);
    }

  private:
    config_t config_;
    population_t population_;
    stats::history<statistics_t> statistics_;
  };

} // namespace soo
} // namespace gal
