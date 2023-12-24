#pragma once

#include "algorithm.hpp"

#include <stop_token>

namespace gal {
namespace moo {

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
                      config::plist<config::select_ptype,
                                    config::rank_ptype,
                                    config::criterion_ptype,
                                    config::observe_ptype>,
                      config::plist<config::tags_ptype>>,

        config::entry<config::rank_ptype,
                      config::plist<config::elite_ptype,
                                    config::cluster_ptype,
                                    config::crowd_ptype,
                                    config::prune_ptype,
                                    config::project_ptype>>,

        config::entry<config::select_ptype,
                      config::plist<config::couple_ptype>>,

        config::entry<config::couple_ptype,
                      config::plist<config::replace_ptype>>>;
  };

  template<typename Config>
  concept algo_config = basic_algo_config<Config> && requires {
    requires pareto_preservance<typename Config::pareto_preservance_t>;

    requires std::same_as<typename Config::pruning_kind_t,
                          gal::cluster_pruning_t> ||
                 std::same_as<typename Config::pruning_kind_t,
                              gal::crowd_pruning_t>;

    requires ranking<typename Config::ranking_t,
                     typename Config::population_t,
                     typename Config::pareto_preservance_t>;
    requires elitism<typename Config::elitism_t,
                     typename Config::population_t,
                     typename Config::pareto_preservance_t>;
    requires clustering<typename Config::clustering_t,
                        typename Config::population_t,
                        typename Config::pareto_preservance_t>;
    requires crowding<typename Config::crowding_t,
                      typename Config::population_t,
                      typename Config::pareto_preservance_t>;
    requires pruning<typename Config::pruning_t, typename Config::population_t>;
    requires projection<typename Config::projection_t,
                        typename Config::population_t,
                        typename Config::pareto_preservance_t>;
  };

  template<algo_config Config>
  class algo {
  public:
    using config_t = Config;
    using population_t = typename config_t::population_t;
    using statistics_t = typename config_t::statistics_t;

  public:
    inline explicit algo(config_t const& config)
        : config_{config}
        , population_{config_.raw_comparator(),
                      config_.scaled_comparator(),
                      config_.population_size(),
                      false}
        , statistics_{config.statistics_depth()} {
    }

    void run(std::stop_token token) {
    }

  private:
    config_t config_;
    population_t population_;
    stats::history<statistics_t> statistics_;
  };

} // namespace moo
} // namespace gal
