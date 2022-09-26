// gal20.cpp : Defines the entry point for the application.
//

#include "gal20.h"

#include "basic.hpp"
#include "coupling.hpp"
#include "criteria.hpp"
#include "crossover.hpp"
#include "mutation.hpp"
#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"

#include <deque>
#include <list>

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

namespace example {
using random_gen = std::mt19937;
using chromosome = std::vector<double>;

class initializator {
public:
  inline explicit initializator(random_gen& generator) noexcept
      : generator_{&generator} {
  }

  inline auto operator()() const noexcept {
    std::uniform_real_distribution<> dist{-10.0, 10.0};
    return chromosome{dist(*generator_), dist(*generator_)};
  }

private:
  random_gen* generator_;
};

struct evaluator {
  inline auto operator()(chromosome const& ch) const noexcept {
    return -std::pow(ch[0], 2.0) - std::pow(ch[1], 2.0);
  }
};

} // namespace example

void setup_alg() {
  example::random_gen gen{};

  gal::config::builder<gal::config::root_ptype, gal::alg::basic_config_map>
      builder{};

  auto c =
      builder.begin()
          .limit_to(20)
          .tag_nothing()
          .make_like(example::initializator{gen})
          .evaluate_against(example::evaluator{})
          .reproduce_using(
              gal::cross::symmetric_singlepoint<example::random_gen>{gen},
              gal::mutate::shuffle<example::random_gen, 1>{gen})
          .scale_none()
          .track_these<gal::stat::generation,
                       gal::stat::extreme_fitness<gal::raw_fitness_tag>,
                       gal::stat::total_fitness<gal::raw_fitness_tag>,
                       gal::stat::average_fitness<gal::raw_fitness_tag>,
                       gal::stat::fitness_deviation<gal::raw_fitness_tag>>(10)
          .stop_when(gal::criteria::generation{100})
          .select_using(
              gal::select::
                  roulette<4, true, gal::raw_fitness_tag, example::random_gen>{
                      gen})
          .couple_like(gal::couple::make_factory<gal::couple::exclusive>(
              gal::couple::
                  reproduction_params<0.8f, 0.2f, std::true_type, std::mt19937>{
                      gen}))
          .replace_with(gal::replace::worst<gal::raw_fitness_tag>{});
}

int main() {

  std::vector<parent_replacement_t> x{};

  pop_t p{true};
  p.replace(std::move(x));

  test_selection_range<pop_t>(std::vector<pop_t::iterator_t>{});

  test_selection<pop_t>(s);
  test_coupling<pop_t>(c);
  test_replacement<pop_t>(r);

  std::mt19937 gen{};

  gal::select::best<2, gal::scaled_fitness_tag> sl1{};
  sl1(p);

  gal::select::random<2, true, std::mt19937> sl2{gen};
  sl2(p);

  gal::select::roulette<2, true, gal::scaled_fitness_tag, std::mt19937> sl3{
      gen};
  sl3(p);

  gal::replace::random<std::mt19937> ro1{gen};
  ro1(p, std::vector<parent_replacement_t>{});

  gal::replace::worst<gal::raw_fitness_tag> ro2{};
  ro2(p, std::vector<parent_replacement_t>{});

  gal::replace::crowd<gal::raw_fitness_tag> ro3{};
  ro3(p, std::vector<parent_replacement_t>{});

  gal::replace::total ro4{};
  ro4(p, std::vector<parent_replacement_t>{});

  gal::replace::parents<10> ro5{};
  ro5(p, std::vector<parent_replacement_t>{});

  using stat_t =
      gal::stat::statistics<pop_t,
                            gal::stat::generation,
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

  auto cp1 = gal::couple::make_factory<gal::couple::overlapping>(rep_p)(rtx);
  cp1(std::vector<pop_t::iterator_t>{});

  auto cp2 = gal::couple::make_factory<gal::couple::field>(rep_p)(rtx);
  cp2(std::vector<pop_t::iterator_t>{});

  gal::stat::history<decltype(stat)> hist{2};
  gal::stat::get_fitness_best_value<gal::raw_fitness_tag> getter{};

  gal::criteria::generation cr1{2};
  cr1(p, hist);

  gal::criteria::value_limit cr2{getter,
                                 [](double fitness) { return fitness >= 10; }};
  cr2(p, hist);

  gal::criteria::value_progress cr3{getter, 2};
  cr3(p, hist);

  gal::cross::symmetric_singlepoint<std::mt19937> cs1{gen};
  cs1(std::vector<int>{}, std::vector<int>{});

  gal::cross::asymmetric_singlepoint<std::mt19937> cs2{gen};
  cs2(std::vector<int>{}, std::vector<int>{});

  gal::cross::symmetric_multipoint<std::mt19937, 2> cs3{gen};
  cs3(std::vector<int>{}, std::vector<int>{});
  cs3(std::list<int>{}, std::list<int>{});
  cs3(std::deque<int>{}, std::deque<int>{});

  gal::cross::asymmetric_multipoint<std::mt19937, 2> cs4{gen};
  cs4(std::vector<int>{}, std::vector<int>{});
  cs4(std::list<int>{}, std::list<int>{});
  cs4(std::deque<int>{}, std::deque<int>{});

  gal::cross::blend cs5{[](int a, int b) { return std::pair{a + b, a - b}; }};
  cs5(std::vector<int>{}, std::vector<int>{});
  cs5(std::list<int>{}, std::list<int>{});
  cs5(std::deque<int>{}, std::deque<int>{});

  std::vector<int> vec_chromo{};
  std::list<int> lst_chromo{};
  std::deque<int> deq_chromo{};

  auto mu1 = gal::mutate::make_flip<2>(gen, [](int& a) { a = 1; });
  mu1(vec_chromo);
  mu1(lst_chromo);
  mu1(deq_chromo);

  auto mu2 = gal::mutate::make_create<2>(gen, []() { return 1; });
  mu2(vec_chromo);
  mu2(lst_chromo);
  mu2(deq_chromo);

  gal::mutate::destroy<std::mt19937, 2> mu3{gen};
  mu3(vec_chromo);
  mu3(lst_chromo);
  mu3(deq_chromo);

  gal::mutate::shuffle<std::mt19937, 2> mu4{gen};
  mu4(vec_chromo);
  mu4(lst_chromo);
  mu4(deq_chromo);

  gal::mutate::interchange<std::mt19937, 2> mu5{gen};
  mu5(vec_chromo);
  mu5(lst_chromo);
  mu5(deq_chromo);

  return 0;
}
