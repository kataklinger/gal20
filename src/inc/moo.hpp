#pragma once

#include "algorithm.hpp"

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
                      config::plist<config::rank_ptype,
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

} // namespace moo
} // namespace gal
