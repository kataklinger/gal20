#pragma once

#include "algorithm.hpp"

#include <stop_token>

namespace gal::moo {

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

      config::entry<config::select_ptype, config::plist<config::couple_ptype>>,

      config::entry<config::couple_ptype,
                    config::plist<config::replace_ptype>>>;
};

template<typename Config>
concept algo_config = basic_algo_config<Config> && requires(Config c) {
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

  { c.ranking() } -> std::convertible_to<typename Config::ranking_t>;
  { c.elitism() } -> std::convertible_to<typename Config::elitism_t>;
  { c.clustering() } -> std::convertible_to<typename Config::clustering_t>;
  { c.crowding() } -> std::convertible_to<typename Config::crowding_t>;
  { c.pruning() } -> std::convertible_to<typename Config::pruning_t>;

  {
    c.projection(std::declval<typename Config::population_context_t&>())
  } -> std::convertible_to<typename Config::projection_t>;
};

template<algo_config Config>
class algo {
public:
  using config_t = Config;
  using population_t = typename config_t::population_t;
  using statistics_t = typename config_t::statistics_t;

private:
  using individual_t = typename population_t::individual_t;
  using evaluation_t = typename individual_t::evaluation_t;

  using population_context_t = typename config_t::population_context_t;
  using reproduction_context_t = typename config_t::reproduction_context_t;

  using pareto_preservance_t = typename config_t::pareto_preservance_t;

  using pareto_t = population_pareto_t<population_t, pareto_preservance_t>;

private:
  inline static constexpr pareto_preservance_t pareto_preservance_tag{};

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
    population_context_t ctx{population_, statistics_};
    reproduction_context_t reproduction{population_,
                                        statistics_,
                                        config_.crossover(),
                                        config_.mutation(),
                                        config_.evaluator()};

    auto coupler = config_.coupling(reproduction);
    auto projector = config_.projection(ctx);

    auto* statistics = &init();
    while (!token.stop_requested() &&
           !std::invoke(config_.criterion(), population_, statistics_)) {
      auto fronts = rank(*statistics);
      stats::count_range(*statistics, rank_count_tag, fronts);

      elite(fronts, *statistics);

      auto clusters = cluster(fronts, *statistics);
      stats::count_range(*statistics, cluster_count_tag, clusters);

      prune(config_.pruning(), clusters, *statistics);

      crowd(fronts, clusters, *statistics);

      prune(config_.pruning(), *statistics);

      project(projector, fronts, clusters, *statistics);

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
  inline auto rank(statistics_t& current) {
    auto timer = stats::start_timer(current, rank_time_tag);
    return std::invoke(config_.ranking(), population_, pareto_preservance_tag);
  }

  inline void elite(pareto_t& sets, statistics_t& current) {
    auto change =
        stats::track_size_change(population_, current, elite_count_tag);
    auto timer = stats::start_timer(current, elite_time_tag);
    std::invoke(config_.elitism(), population_, sets);
  }

  inline auto cluster(pareto_t& sets, statistics_t& current) {
    auto timer = stats::start_timer(current, cluster_time_tag);
    return std::invoke(config_.clustering(), population_, sets);
  }

  inline void
      crowd(pareto_t& sets, cluster_set& clusters, statistics_t& current) {
    auto timer = stats::start_timer(current, cluster_time_tag);
    return std::invoke(config_.crowding(), population_, sets, clusters);
  }

  template<cluster_pruning Pruning>
  inline void prune(Pruning const& operation,
                    cluster_set& clusters,
                    statistics_t& current) {
    auto change =
        stats::track_size_change(population_, current, prune_count_tag);
    auto timer = stats::start_timer(current, prune_time_tag);
    std::invoke(operation, population_, clusters);
  }

  template<typename Pruning>
  inline void prune(Pruning const& /*unused*/,
                    cluster_set& /*unused*/,
                    statistics_t& /*unused*/) noexcept {
  }

  template<crowd_pruning Pruning>
  inline void prune(Pruning const& operation, statistics_t& current) {
    auto change =
        stats::track_size_change(population_, current, prune_count_tag);
    auto timer = stats::start_timer(current, prune_time_tag);
    std::invoke(operation, population_);
  }

  template<typename Pruning>
  inline void prune(Pruning const& /*unused*/,
                    statistics_t& /*unused*/) noexcept {
  }

  template<typename Projection>
  inline void project(Projection const& operation,
                      pareto_t& sets,
                      cluster_set& clusters,
                      statistics_t& current) {
    auto timer = stats::start_timer(current, project_time_tag);
    return std::invoke(operation, sets, clusters);
  }

  inline auto select(statistics_t& current) {
    auto timer = stats::start_timer(current, selection_time_tag);
    return std::invoke(config_.selection(), population_);
  }

  template<std::ranges::range Selected,
           coupling<population_t, Selected> Coupling>
  inline auto
      couple(Selected&& selected, Coupling&& operation, statistics_t& current) {
    auto coupling_time = stats::start_timer(current, coupling_time_tag);
    return std::invoke(std::forward<Coupling>(operation),
                       std::forward<Selected>(selected));
  }

  template<std::ranges::range Offspring>
  inline auto replace(Offspring&& offspring, statistics_t& current) {
    auto timer = stats::start_timer(current, replacement_time_tag);
    return std::invoke(
        config_.replacement(), population_, std::forward<Offspring>(offspring));
  }

  inline auto& init() {
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

} // namespace gal::moo
