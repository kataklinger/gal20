// gal20.cpp : Defines the entry point for the application.
//

#include "gal20.h"

#include "basic.hpp"
#include "coupling.hpp"
#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"

#include <tuple>

using pop_t = gal::population<int, double, double, int>;

struct parent_replacement_t
    : std::tuple<pop_t::iterator_t, pop_t::individual_t> {
  using tuple::tuple;
};

pop_t::iterator_t& get_parent(parent_replacement_t& x) {
  return get<0>(x);
}

pop_t::individual_t& get_child(parent_replacement_t& x) {
  return get<1>(x);
}

pop_t::individual_t& get_child(parent_replacement_t&& x) {
  return get<1>(x);
}

template<typename Population,
         gal::selection_range<typename Population::iterator_t> Range>
void test_selection_range(Range&& r) {
}

template<typename Population, gal::selection<Population> Selection>
void test_selection(Selection&& s) {
}

template<typename Population,
         gal::coupling<Population, std::vector<pop_t::iterator_t>> Coupling>
void test_coupling(Coupling&& s) {
}

template<
    typename Population,
    gal::replacement<Population, std::vector<parent_replacement_t>> Replacement>
void test_replacement(Replacement&& r) {
}

std::vector<pop_t::iterator_t> s(pop_t const& p) {
  return {};
}

std::vector<parent_replacement_t> c(std::vector<pop_t::iterator_t>&& p) {
  return {};
}

std::vector<pop_t::individual_t> r(pop_t& p,
                                   std::vector<parent_replacement_t>&& v) {
  return {};
}

struct test_crossover {
  std::pair<int, int> operator()(int, int) const {
    return {0, 0};
  }
};

struct test_mutation {
  void operator()(int&) const {
  }
};

struct test_evaluator {
  double operator()(int) const {
    return 0.0;
  }
};

int main() {

  std::vector<parent_replacement_t> x{};

  pop_t p{true};
  p.replace(std::move(x));

  test_selection_range<pop_t>(std::vector<pop_t::iterator_t>{});

  test_selection<pop_t>(s);
  test_coupling<pop_t>(c);
  test_replacement<pop_t>(r);

  std::mt19937 gen{};

  gal::replace::random<10, std::mt19937> ro1{gen};
  ro1(p, std::vector<parent_replacement_t>{});

  gal::replace::worst<10, gal::raw_fitness_tag> ro2{};
  ro2(p, std::vector<parent_replacement_t>{});

  gal::replace::crowd<gal::raw_fitness_tag> ro3{};
  ro3(p, std::vector<parent_replacement_t>{});

  gal::replace::total ro4{};
  ro4(p, std::vector<parent_replacement_t>{});

  gal::replace::parents<10> ro5{};
  ro5(p, std::vector<parent_replacement_t>{});

  using stat_t =
      gal::stat::statistics<pop_t,
                            gal::stat::extreme_fitness<gal::raw_fitness_tag>,
                            gal::stat::total_fitness<gal::raw_fitness_tag>,
                            gal::stat::average_fitness<gal::raw_fitness_tag>,
                            gal::stat::fitness_deviation<gal::raw_fitness_tag>>;
  stat_t stat{};

  using ctx_t = gal::population_context<pop_t, stat_t>;

  ctx_t ctx{p, stat};

  auto sc0 = gal::scale::factory<gal::scale::top, 2, 1.5>{}(ctx);

  gal::scale::top<ctx_t, 5, 1.5> sc1{ctx};
  sc1();
  sc1(0, p.individuals()[0]);

  gal::scale::exponential<ctx_t, 1.0025> sc2{ctx};
  sc2();
  sc2(0, p.individuals()[0]);

  gal::scale::power<ctx_t, 2> sc3{ctx};
  sc3(p.individuals()[0]);

  gal::scale::window<ctx_t> sc4{ctx};
  sc4(0, p.individuals()[0]);

  gal::scale::ranked<ctx_t, 1> sc5{ctx};
  sc5();
  sc5(0, p.individuals()[0]);

  gal::scale::sigma<ctx_t> sc6{ctx};
  sc6(0, p.individuals()[0]);

  gal::scale::linear<ctx_t, 1.0025> sc7{ctx};
  sc7();
  sc7(0, p.individuals()[0]);

  using rtx_t =
      gal::reproduction_context_with_scaling<pop_t,
                                             stat_t,
                                             test_crossover,
                                             test_mutation,
                                             test_evaluator,
                                             gal::scale::power<ctx_t, 2>>;
  rtx_t rtx{p, stat, test_crossover{}, test_mutation{}, test_evaluator{}, sc3};

  gal::couple::reproduction_params<0.8f, 0.2f, std::true_type, std::mt19937>
      rep_p{gen};

  auto cp0 = gal::couple::make_factory<gal::couple::exclusive>(rep_p)(rtx);

  cp0(std::vector<pop_t::iterator_t>{});

  return 0;
}
