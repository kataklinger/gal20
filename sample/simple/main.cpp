
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

#include <format>
#include <iostream>

namespace f1 {

using chromosome_t = std::array<double, 2>;
using fitness_t = double;

struct spawn {
  std::mt19937* rng_;

  inline chromosome_t operator()() const noexcept {
    std::uniform_real_distribution<> dist{-10., 10.};
    return {dist(*rng_), dist(*rng_)};
  }
};

struct evaluate {
  inline fitness_t operator()(chromosome_t const& c) const noexcept {
    return std::pow(c[0], 2.) + std::pow(c[1], 2.);
  }
};

struct observe {
  template<typename Population, typename History>
  inline void operator()(Population const& population,
                         History const& history) const {
    std::cout << std::format("{:-^32}", history.current().generation_value())
              << std::endl;

    for (int idx = 1; auto&& individual : population.individuals()) {
      auto&& c = individual.chromosome();
      auto&& f = individual.evaluation().raw();

      std::cout << std::format(
                       "#{:3}| {:7.4f}, {:7.4f} | {:7.4f}", idx, c[0], c[1], f)
                << std::endl;

      ++idx;
    }

    std::cout << std::format("{:-^32}", 'x') << std::endl;
  }
};

} // namespace f1

namespace f1f2 {

using chromosome_t = std::array<double, 2>;
using fitness_t = std::array<double, 2>;

struct spawn {
  std::mt19937* rng_;

  inline chromosome_t operator()() const noexcept {
    std::uniform_real_distribution<> dist{-10., 10.};
    return {dist(*rng_), dist(*rng_)};
  }
};

struct evaluate {
  inline fitness_t operator()(chromosome_t const& c) const noexcept {
    return {std::pow(c[0] + 1., 2.), std::pow(c[1] - 1., 2.)};
  }
};

struct observe {
  template<typename Population, typename History>
  inline void operator()(Population const& /*population*/,
                         History const& /*history*/) const {
  }
};

} // namespace f1f2

void simple() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<soo::algo_config_map>()
      .begin()
      .limit(20)
      .tag()
      .spawn(f1::spawn{&rng})
      .evaluate(f1::evaluate{},
                gal::minimize{gal::min_floatingpoint_three_way{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale()
      .track<stats::generation,
             stats::extreme_fitness_raw,
             stats::total_fitness_raw,
             stats::average_fitness_raw,
             stats::fitness_deviation_raw>(10)
      .stop(criteria::generation_limit{100})
      .select(select::random{select::unique<4>, rng})
      //.select(select::roulette_raw{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::worst_raw{})
      .observe(observe{generation_event, f1::observe{}})
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
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
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
      .replace(replace::total{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void nsga_ii() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, int_rank_t, crowd_density_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<std::tuple<std::size_t, double>>(
          [](auto&, auto&) { return std::weak_ordering::less; })
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::level{})
      .elite(elite::relaxed{})
      .cluster(cluster::none{})
      .crowd(crowd::distance{})
      .prune(prune::global_worst<int_rank_t>{})
      .project(project::factory<project::merge, int_rank_t>{})
      .select(
          select::tournament_scaled{select::unique<2>, select::rounds<2>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void spea() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, real_rank_t, cluster_label, prune_state_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::strength{})
      .elite(elite::relaxed{})
      .cluster(cluster::linkage{})
      .crowd(crowd::none{})
      .prune(prune::cluster_edge{})
      .project(project::factory<project::truncate, real_rank_t>{})
      .select(select::roulette_scaled{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void spea_ii() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, int_rank_t, crowd_density_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::accumulated_strength{})
      .elite(elite::relaxed{})
      .cluster(cluster::none{})
      .crowd(crowd::neighbor{})
      .prune(prune::global_worst<int_rank_t>{})
      .project(project::factory<project::translate, int_rank_t>{})
      .select(select::roulette_scaled{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void rdga() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, int_rank_t, crowd_density_t, cluster_label>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::accumulated_level{})
      .elite(elite::relaxed{})
      .cluster(cluster::adaptive_hypergrid<10, 10>{})
      .crowd(crowd::cluster{})
      .prune(prune::none{})
      .project(project::factory<project::alternate, int_rank_t>{})
      .select(select::roulette_scaled{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void pesa() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t,
           bin_rank_t,
           crowd_density_t,
           cluster_label,
           prune_state_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::binary{})
      .elite(elite::strict{})
      .cluster(cluster::hypergrid<std::array<double, 2>, 0.1, 0.1>{})
      .crowd(crowd::cluster{})
      .prune(prune::cluster_random{rng})
      .project(project::factory<project::truncate, crowd_density_t>{})
      .select(select::roulette_scaled{select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void pesa_ii() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t, bin_rank_t, cluster_label, prune_state_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale()
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::binary{})
      .elite(elite::strict{})
      .cluster(cluster::hypergrid<std::array<double, 2>, 0.1, 0.1>{})
      .crowd(crowd::none{})
      .prune(prune::cluster_random{rng})
      .project(project::factory<project::none>{})
      .select(select::cluster{
          gal::select::shared<cluster_label>, select::unique<4>, rng})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

void paes() {
  using namespace gal;

  std::mt19937 rng{};
  std::stop_token stop{};

  std::uniform_real_distribution<> dist{-10.0, 10.0};

  config::for_map<moo::algo_config_map>()
      .begin()
      .limit(20)
      .tag<frontier_level_t,
           bin_rank_t,
           crowd_density_t,
           cluster_label,
           prune_state_t,
           lineage_t>()
      .spawn(f1f2::spawn{&rng})
      .evaluate(f1f2::evaluate{}, gal::dominate{std::less{}})
      .reproduce(cross::symmetric_singlepoint{rng},
                 mutate::make_simple_flip<1>(rng, dist))
      .scale<double>(gal::floatingpoint_three_way{})
      .track<stats::generation>(10)
      .stop(criteria::generation_limit{100})
      .rank<pareto_preserved_t>(gal::rank::binary{})
      .elite(elite::strict{})
      .cluster(cluster::adaptive_hypergrid<10, 10>{})
      .crowd(crowd::cluster{})
      .prune(prune::cluster_random{rng})
      .project(project::factory<project::truncate, crowd_density_t>{})
      .select(select::lineal_scaled{})
      .couple(couple::parametrize<couple::exclusive, 0.8f, 0.2f, true>(rng))
      .replace(replace::insert{})
      .observe(observe{generation_event, f1f2::observe{}})
      .build<moo::algo>()
      .run(stop);
}

int main() {
  auto render_line = [](int no, std::string_view name) {
    return std::format(
        "({:2}) | MOO ({:6}) | y1 = (x1 + 1) ^ 2, y2 = (x2 + 1) ^ 2\n",
        no,
        name);
  };

  while (true) {
    std::cout << "Algorithms:" << std::endl;
    std::cout << "( 1) | SOO          | z = x ^ 2 + y ^ 2" << std::endl;
    std::cout << render_line(2, "NSGA");
    std::cout << render_line(3, "NSGAII");
    std::cout << render_line(4, "SPEA");
    std::cout << render_line(5, "SPEAII");
    std::cout << render_line(6, "RDGA");
    std::cout << render_line(7, "PESA");
    std::cout << render_line(8, "PESAII");
    std::cout << render_line(9, "PAES");
    std::cout << "Pick algorithm [1-9]: ";

    int choice = -1;
    std::cin >> choice;

    switch (choice) {
    case 1: simple(); break;
    case 2: nsga(); break;
    case 3: nsga_ii(); break;
    case 4: spea(); break;
    case 5: spea_ii(); break;
    case 6: rdga(); break;
    case 7: pesa(); break;
    case 8: pesa_ii(); break;
    case 9: paes(); break;
    }
  }

  simple();
  return 0;
}
