#pragma once

#include "config.hpp"

#include <functional>
#include <stop_token>

namespace gal {
namespace simple {
  template<typename Builder>
  struct basic_algorithm_scaling_cond
      : is_empty_fitness<typename Builder::scaled_fitness_t> {};

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
              basic_algorithm_scaling_cond,
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

  template<algorithm_config Config>
  class algorithm {
  public:
    using config_t = Config;
    using population_t = typename config_t::population_t;
    using statistics_t = typename config_t::statistics_t;

  private:
    using individual_t = typename population_t::individual_t;
    using evaluation_t = typename individual_t::evaluation_t;
    using scaling_t = typename config_t::scaling_t;

    using reproduction_context_t = typename config_t::reproduction_context_t;

  public:
    inline explicit algorithm(config_t const& config)
        : config_{config}
        , population_{population_.population_size(),
                      config_t::is_stable_scaling_t::value} {
    }

    void run(std::stop_token token) {
      auto scaling = config_.scaling({population_, statistics_});
      constexpr auto global_scaling = typename config_t::is_global_scaling_t{};

      auto coupling =
          config_.coupling(get_coupling_context(scaling, global_scaling));

      for (auto const* stats = &init();
           !token.stop_requested() && !config_.criterion(population_, *stats);
           stats = &statistics_.next(population_)) {
        scale(scaling, global_scaling);

        auto selected = std::invoke(config_.selection(), population_);
        auto offspring = std::invoke(coupling, offspring);
        auto replaced =
            std::invoke(config_.replacement(), population_, offspring);

        population_.trim();
      }
    }

  private:
    auto const& init() {
      std::ranges::generate_n(
          std::back_inserter(population_.individuals()),
          population_.target_size(),
          [&config_] {
            auto chromosome = config_.initializator()();

            evaluation_t evaluation{};
            config_.evaluator()(chromosome, evaluation.raw());

            return individual_t{std::move(chromosome), std::move(evaluation)};
          });

      return statistics_.next(population_);
    }

    void scale(scaling_t& scaling, std::true_type /*unused*/) {
      std::invoke(scaling);

      for (auto& individual : population_.individuals()) {
        auto& evaluation = individual.evaluation();

        std::invoke(scaling,
                    individual.chromosome(),
                    evaluation.raw(),
                    evaluation.scaled());
      }
    }

    inline void scale(scaling_t& scaling, std::false_type /*unused*/) noexcept {
    }

    inline auto get_coupling_context(scaling_t const& scaling,
                                     std::true_type /*unused*/) {
      return reproduction_context_t{
          config_.crossover(), config_.mutation(), config_.evaluator()};
    }

    inline auto get_coupling_context(scaling_t const& scaling,
                                     std::false_type /*unused*/) {
      return reproduction_context_t{config_.crossover(),
                                    config_.mutation(),
                                    config_.evaluator(),
                                    scaling};
    }

  private:
    config_t config_;
    population_t population_;
    stats::history<statistics_t> statistics_;
  };

} // namespace simple
} // namespace gal
