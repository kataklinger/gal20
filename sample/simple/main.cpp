﻿
#include "coupling.hpp"

#include "criteria.hpp"
#include "observing.hpp"

#include "crossover.hpp"
#include "mutation.hpp"

#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"

#include "clustering.hpp"
#include "crowding.hpp"
#include "elitism.hpp"
#include "projection.hpp"
#include "pruning.hpp"
#include "ranking.hpp"

#include "moo.hpp"
#include "soo.hpp"

void simple() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<soo::algo_config_map>()
      .begin()
      .limit(20)
      .tag()
      .spawn([&rng, &dist] {
        return std::vector<double>{dist(rng), dist(rng)};
      })
      .evaluate([](auto& c) { return std::pow(c[0] + c[1], 2.0); },
                gal::floatingpoint_three_way{})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale()
      .track<stats::generation,
             stats::extreme_fitness_raw,
             stats::total_fitness_raw,
             stats::average_fitness_raw,
             stats::fitness_deviation_raw>(10)
      .stop(criteria::generation_limit{100})
      .select(select::roulette_raw{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::worst_raw{})
      .observe(observe{generation_event,
                       [](auto const& /*unused*/, auto const& /*unused*/) {}})
      .build<soo::algo>()
      .run(stop);
}

void nsga() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, int_rank_t, crowd_density_t>()
      .spawn([&rng, &dist] {
        return std::vector<double>{dist(rng), dist(rng)};
      })
      .evaluate(
          [](auto& c) {
            return std::array{std::pow(c[0], 2.), std::pow(c[1], 2.)};
          },
          gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::level{})
      .elite(elite::relaxed{})
      .cluster(cluster::none{})
      .crowd(crowd::sharing{1., 2., [](auto&, auto&) { return 0.; }})
      .prune(prune::none{})
      .project(project::factory<project::scale, int_rank_t>{})
      .select(select::roulette_scaled{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event,
                       [](auto const& /*unused*/, auto const& /*unused*/) {}})
      .build<moo::algo>()
      .run(stop);
}

int main() {
  simple();
  return 0;
}
