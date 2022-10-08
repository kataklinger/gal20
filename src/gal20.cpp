// gal20.cpp : Defines the entry point for the application.
//

#include "coupling.hpp"
#include "criteria.hpp"
#include "crossover.hpp"
#include "mutation.hpp"
#include "observe.hpp"
#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"
#include "soo.hpp"

namespace example {

using random_gen = std::mt19937;
using chromosome = std::vector<double>;

const std::uniform_real_distribution<> dist{-10.0, 10.0};

class initializator {
public:
  inline explicit initializator(random_gen& generator) noexcept
      : generator_{&generator} {
  }

  inline auto operator()() const noexcept {
    return chromosome{dist(*generator_), dist(*generator_)};
  }

private:
  random_gen* generator_;
};

struct evaluator {
  inline auto operator()(chromosome const& chromosome) const noexcept {
    return std::pow(chromosome[0] + chromosome[1], 2.0);
  }
};

} // namespace example

int main() {
  using namespace gal;

  example::random_gen rng{};
  std::stop_token stop{};

  config::for_map<soo::algo_config_map>()
      .begin()
      .limit(20)
      .tag()
      .spawn(example::initializator{rng})
      .evaluate(example::evaluator{}, std::less{})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, example::dist))
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
