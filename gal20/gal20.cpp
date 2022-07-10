// gal20.cpp : Defines the entry point for the application.
//

#include "gal20.h"

#include "config.hpp"

#include <tuple>

namespace gal {

namespace algo {

  template<typename Config>
  concept scaling_config =
      std::same_as<typename Config::scaled_fitness_t, empty_fitness> ||
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

    std::derived_from<typename Config::only_improving_mutation_t,
                      std::true_type> ||
        std::derived_from<typename Config::only_improving_mutation_t,
                          std::false_type>;

    std::derived_from<typename Config::is_global_scaling_t, std::true_type> ||
        std::derived_from<typename Config::is_global_scaling_t,
                          std::false_type>;

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

    {
      c.reproduction_context()
      } -> std::convertible_to<typename Config::reproduction_context_t>;

    {
      c.population_context()
      } -> std::same_as<typename Config::population_context_t>;

    { c.selection() } -> std::convertible_to<typename Config::selection_t>;

    {
      c.coupling(std::declval<typename Config::reproduction_context_t>())
      } -> std::convertible_to<typename Config::coupling_t>;

    { c.criterion() } -> std::convertible_to<typename Config::criterion_t>;
  };

  template<algorithm_config Config>
  class algorithm {
  private:
    using config_t = Config;

  public:
  };

} // namespace algo

} // namespace gal

using pop_t = gal::population<int, int, int, int>;

struct parent_replacement_t
    : std::tuple<pop_t::const_iterator_t, pop_t::individual_t> {
  using tuple::tuple;
};

pop_t::const_iterator_t get_parent(parent_replacement_t x) {
  return get<0>(x);
}

pop_t::individual_t get_child(parent_replacement_t x) {
  return get<1>(x);
}

template<typename Population,
         gal::selection_range<typename Population::const_iterator_t> Range>
void test_selection_range(Range&& r) {
}

template<typename Population, gal::selection<Population> Selection>
void test_selection(Selection&& s) {
}

template<
    typename Population,
    gal::coupling<Population, std::vector<pop_t::const_iterator_t>> Coupling>
void test_coupling(Coupling&& s) {
}

template<
    typename Population,
    gal::replacement<Population, std::vector<parent_replacement_t>> Replacement>
void test_replacement(Replacement&& r) {
}

std::vector<pop_t::const_iterator_t> s(pop_t const& p) {
  return {};
}

std::vector<parent_replacement_t> c(std::vector<pop_t::const_iterator_t>&& p) {
  return {};
}

std::vector<pop_t::individual_t> r(pop_t& p,
                                   std::vector<parent_replacement_t>&& v) {
  return {};
}

int main() {

  std::vector<parent_replacement_t> x{};

  pop_t p{};
  p.replace(std::move(x));

  test_selection_range<pop_t>(std::vector<pop_t::const_iterator_t>{});

  test_selection<pop_t>(s);
  test_coupling<pop_t>(c);
  test_replacement<pop_t>(r);

  return 0;
}
