
#include "coupling.hpp"
#include "criteria.hpp"
#include "crossover.hpp"
#include "mutation.hpp"
#include "observing.hpp"
#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"
#include "soo.hpp"

const std::uniform_real_distribution<> dist{-10.0, 10.0};

int main() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  config::for_map<soo::algo_config_map>()
      .begin()
      .limit(20)
      .tag()
      .spawn([&rng] {
        return std::vector<double>{dist(rng), dist(rng)};
      })
      .evaluate([](auto& c) { return std::pow(c[0] + c[1], 2.0); }, std::less{})
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
      .couple(couple::factorize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::worst_raw{})
      .observe(observe{soo::generation_event,
                       [](auto const& pop, auto const& hist) {}})
      .build<soo::algo>()
      .run(stop);

  return 0;
}
