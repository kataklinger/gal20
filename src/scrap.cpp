
#include "clustering.hpp"
#include "coupling.hpp"
#include "criteria.hpp"
#include "crossover.hpp"
#include "crowding.hpp"
#include "elitism.hpp"
#include "mutation.hpp"
#include "observing.hpp"
#include "pareto.hpp"
#include "projection.hpp"
#include "pruning.hpp"
#include "ranking.hpp"
#include "replacement.hpp"
#include "scaling.hpp"
#include "selection.hpp"
#include "soo.hpp"

#include <deque>
#include <functional>
#include <list>

using pop_t = gal::population<int,
                              double,
                              gal::max_floatingpoint_three_way,
                              double,
                              gal::max_floatingpoint_three_way,
                              std::tuple<int, gal::lineage_t>>;

using mo_fit_t = std::array<int, 2>;

using mo_pop_t = gal::population<int,
                                 mo_fit_t,
                                 gal::dominate<std::less<>>,
                                 double,
                                 gal::max_floatingpoint_three_way,
                                 std::tuple<gal::frontier_level_t,
                                            gal::bin_rank_t,
                                            gal::int_rank_t,
                                            gal::real_rank_t,
                                            gal::cluster_label,
                                            gal::crowd_density_t,
                                            gal::prune_state_t>>;

using mo_m_pop_t = gal::population<int,
                                   mo_fit_t,
                                   gal::dominate<std::less<>>,
                                   std::tuple<std::size_t, double>,
                                   std::compare_three_way,
                                   std::tuple<gal::frontier_level_t,
                                              gal::bin_rank_t,
                                              gal::int_rank_t,
                                              gal::real_rank_t,
                                              gal::cluster_label,
                                              gal::crowd_density_t,
                                              gal::prune_state_t>>;

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
void test_selection_range(Range&& /*unused*/) {
}

template<typename Population, gal::selection<Population> Selection>
void test_selection(Selection&& /*unused*/) {
}

template<typename Population,
         gal::coupling<Population, std::vector<pop_t::iterator_t>> Coupling>
void test_coupling(Coupling&& /*unused*/) {
}

template<
    typename Population,
    gal::replacement<Population, std::vector<parent_replacement_t>> Replacement>
void test_replacement(Replacement&& /*unused*/) {
}

std::vector<pop_t::iterator_t> s(pop_t const& /*unused*/) {
  return {};
}

std::vector<parent_replacement_t>
    c(std::vector<pop_t::iterator_t>&& /*unused*/) {
  return {};
}

std::vector<pop_t::individual_t>
    r(pop_t& /*unused*/, std::vector<parent_replacement_t>&& /*unused*/) {
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

void test_ground() {
  std::vector<parent_replacement_t> x{};

  pop_t p{{}, {}, true};
  p.replace(std::move(x));

  test_selection_range<pop_t>(std::vector<pop_t::iterator_t>{});

  test_selection<pop_t>(s);
  test_coupling<pop_t>(c);
  test_replacement<pop_t>(r);

  std::mt19937 gen{};

  gal::select::best<2, gal::scaled_fitness_tag> sl1{};
  sl1(p);

  gal::select::random sl2{gal::select::unique<2>, gen};
  sl2(p);

  gal::select::tournament_scaled sl3{
      gal::select::unique<2>, gal::select::rounds<2>, gen};
  sl3(p);

  gal::select::roulette_scaled sl4{gal::select::unique<2>, gen};
  sl4(p);

  gal::select::lineal_raw sl6{};
  sl6(p);

  gal::replace::random_raw ro1{gen, gal::countable<1>};
  ro1(p, std::vector<parent_replacement_t>{});

  gal::replace::worst<gal::raw_fitness_tag> ro2{};
  ro2(p, std::vector<parent_replacement_t>{});

  gal::replace::crowd<gal::raw_fitness_tag> ro3{};
  ro3(p, std::vector<parent_replacement_t>{});

  gal::replace::total ro4{};
  ro4(p, std::vector<parent_replacement_t>{});

  gal::replace::parents ro5{};
  ro5(p, std::vector<parent_replacement_t>{});

  gal::replace::append ro6{};
  ro6(p, std::vector<parent_replacement_t>{});

  using stat_t = gal::stats::statistics<
      pop_t,
      gal::stats::generation,
      gal::stats::fitness_deviation<gal::raw_fitness_tag>,
      gal::stats::average_fitness<gal::raw_fitness_tag>,
      gal::stats::total_fitness<gal::raw_fitness_tag>,
      gal::stats::extreme_fitness<gal::raw_fitness_tag>>;
  stat_t stat{};
  gal::stats::history<decltype(stat)> hist{2};

  using ctx_t = gal::population_context<pop_t, stat_t>;

  ctx_t ctx{p, hist};

  using namespace gal::literals;

  auto sc0 = gal::scale::factory<gal::scale::top, 2, 1.5_dc>{}(ctx);
  sc0();

  gal::scale::top<ctx_t, 5, 1.5_dc> sc1{ctx};
  sc1();
  sc1(0, p.individuals()[0]);

  gal::scale::exponential<ctx_t, 1.0025_dc> sc2{ctx};
  sc2();
  sc2(0, p.individuals()[0]);

  gal::scale::power<ctx_t, 2._dc> sc3{ctx};
  sc3(0, p.individuals()[0]);

  gal::scale::window<ctx_t> sc4{ctx};
  sc4(0, p.individuals()[0]);

  gal::scale::ranked<ctx_t, 1._dc> sc5{ctx};
  sc5();
  sc5(0, p.individuals()[0]);

  gal::scale::sigma<ctx_t> sc6{ctx};
  sc6(0, p.individuals()[0]);

  gal::scale::linear<ctx_t, 1.0025_dc> sc7{ctx};
  sc7();
  sc7(0, p.individuals()[0]);

  using rtx_t =
      gal::reproduction_context_with_scaling<pop_t,
                                             stat_t,
                                             test_crossover,
                                             test_mutation,
                                             test_evaluator,
                                             gal::scale::power<ctx_t, 2._dc>>;
  rtx_t rtx{p, hist, test_crossover{}, test_mutation{}, test_evaluator{}, sc3};

  gal::couple::reproduction_params<0.8_fc, 0.2_fc, std::true_type, std::mt19937>
      rep_p{gen};

  auto cp0 = gal::couple::parametrize<gal::couple::exclusive>(rep_p)(rtx);
  cp0(std::vector<pop_t::iterator_t>{});

  auto cp1 = gal::couple::parametrize<gal::couple::overlapping>(rep_p)(rtx);
  cp1(std::vector<pop_t::iterator_t>{});

  auto cp2 = gal::couple::parametrize<gal::couple::field>(rep_p)(rtx);
  cp2(std::vector<pop_t::iterator_t>{});

  auto cp3 = gal::couple::local{rtx};
  cp3(std::vector<pop_t::iterator_t>{});

  gal::criteria::generation_limit cr1{2};
  cr1(p, hist);

  gal::criteria::value_limit cr2{gal::stats::get_raw_fitness_best_value{},
                                 [](double fitness) { return fitness >= 10; }};
  cr2(p, hist);

  gal::criteria::value_progress cr3{
      gal::stats::get_raw_fitness_best_value{}, std::less{}, 2};
  cr3(p, hist);

  gal::index_generator igen{gen};

  gal::cross::symmetric_singlepoint cs1{igen};
  cs1(std::vector<int>{}, std::vector<int>{});

  gal::cross::asymmetric_singlepoint cs2{igen};
  cs2(std::vector<int>{}, std::vector<int>{});

  gal::cross::symmetric_multipoint cs3{igen, gal::countable<2>};
  cs3(std::vector<int>{}, std::vector<int>{});
  cs3(std::list<int>{}, std::list<int>{});
  cs3(std::deque<int>{}, std::deque<int>{});

  gal::cross::asymmetric_multipoint cs4{igen, gal::countable<2>};
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

  gal::mutate::flip mu1{igen, gal::countable<2>, [](int& a) { a = 1; }};
  mu1(vec_chromo);
  mu1(lst_chromo);
  mu1(deq_chromo);

  gal::mutate::create mu2{igen, gal::countable<2>, []() { return 1; }};
  mu2(vec_chromo);
  mu2(lst_chromo);
  mu2(deq_chromo);

  gal::mutate::destroy mu3{igen, gal::countable<2>};
  mu3(vec_chromo);
  mu3(lst_chromo);
  mu3(deq_chromo);

  gal::mutate::shuffle mu4{igen, gal::countable<2>};
  mu4(vec_chromo);
  mu4(lst_chromo);
  mu4(deq_chromo);

  gal::mutate::interchange mu5{igen, gal::countable<2>};
  mu5(vec_chromo);
  mu5(lst_chromo);
  mu5(deq_chromo);

  using fit_t = std::vector<float>;

  using indv_t =
      gal::individual<int, fit_t, gal::empty_fitness, gal::empty_tags>;

  struct dom_flag {
    inline bool get(indv_t const&) const noexcept {
      return true;
    }

    inline void set(indv_t&) const noexcept {
    }
  };

  mo_pop_t mp{{}, {}, true};

  gal::select::cluster sl5{
      gal::select::shared<int>, gal::select::unique<2>, gen};
  sl5(mp);

  gal::rank::binary rk0{};
  rk0(mp, gal::pareto_preserved_tag);
  rk0(mp, gal::pareto_reduced_tag);
  rk0(mp, gal::pareto_nondominated_tag);
  rk0(mp, gal::pareto_erased_tag);

  gal::rank::level rk1{};
  rk1(mp, gal::pareto_preserved_tag);
  rk1(mp, gal::pareto_reduced_tag);
  rk1(mp, gal::pareto_nondominated_tag);
  rk1(mp, gal::pareto_erased_tag);

  gal::rank::accumulated_level rk2{};
  rk2(mp, gal::pareto_preserved_tag);
  rk2(mp, gal::pareto_reduced_tag);
  rk2(mp, gal::pareto_nondominated_tag);
  rk2(mp, gal::pareto_erased_tag);

  gal::rank::strength rk3{};
  rk3(mp, gal::pareto_preserved_tag);
  rk3(mp, gal::pareto_reduced_tag);
  rk3(mp, gal::pareto_nondominated_tag);
  rk3(mp, gal::pareto_erased_tag);

  gal::rank::accumulated_strength rk4{};
  rk4(mp, gal::pareto_preserved_tag);
  rk4(mp, gal::pareto_reduced_tag);
  rk4(mp, gal::pareto_nondominated_tag);
  rk4(mp, gal::pareto_erased_tag);

  gal::population_pareto_t<mo_pop_t, gal::pareto_preserved_t> pps{1};
  gal::population_pareto_t<mo_pop_t, gal::pareto_reduced_t> prs{1};
  gal::population_pareto_t<mo_pop_t, gal::pareto_nondominated_t> pns{1};
  gal::population_pareto_t<mo_pop_t, gal::pareto_erased_t> pes{1};

  gal::elite::strict el0{};
  el0(mp, pps);
  el0(mp, prs);
  el0(mp, pns);

  gal::elite::relaxed el1{};
  el1(mp, pps);

  gal::cluster::none cl0{};
  cl0(mp, pps);

  gal::cluster::linkage cl1{};
  cl1(mp, pps);

  gal::cluster::hypergrid<mo_fit_t, 2, 2> cl2{};
  cl2(mp, pps);

  gal::cluster::adaptive_hypergrid<2, 2> cl3{};
  cl3(mp, pps);

  gal::cluster_set cls{};

  gal::prune::none pr0{};
  pr0(mp);

  gal::prune::global_worst<gal::int_rank_t> pr1{};
  pr1(mp);

  std::mt19937 rng{};

  gal::prune::cluster_random<std::mt19937> pr2{rng};
  pr2(mp, cls);

  gal::prune::cluster_edge pr3{};
  pr3(mp, cls);

  gal::crowd::none cw0{};
  cw0(mp, pps, cls);

  gal::crowd::sharing cw1{1.0, 2.0, [](int, int) { return 0.; }};
  cw1(mp, pps, cls);

  gal::crowd::distance cw2{};
  cw2(mp, pps, cls);

  gal::crowd::neighbor cw3{};
  cw3(mp, pps, cls);

  gal::crowd::cluster cw4{};
  cw4(mp, pps, cls);

  using mo_ctx_t = gal::population_context<mo_pop_t, stat_t>;
  mo_ctx_t mo_ctx{mp, hist};

  gal::project::scale<gal::int_rank_t> pj0{};
  pj0(mo_ctx, pps, cls);

  gal::project::translate<gal::int_rank_t> pj1{};
  pj1(mo_ctx, pps, cls);

  mo_m_pop_t mmp{{}, {}, true};
  gal::population_pareto_t<mo_m_pop_t, gal::pareto_preserved_t> mpps{1};

  using mo_m_ctx_t = gal::population_context<mo_m_pop_t, stat_t>;
  mo_m_ctx_t mo_m_ctx{mmp, hist};

  gal::project::merge<gal::int_rank_t> pj2{};
  pj2(mo_m_ctx, mpps, cls);

  gal::project::truncate<gal::int_rank_t> pj3{};
  pj3(mo_ctx, pps, cls);

  gal::project::truncate<gal::crowd_density_t> pj4{};
  pj4(mo_ctx, pps, cls);

  gal::project::alternate<gal::int_rank_t> pj5{};
  pj5(mo_ctx, pps, cls);
}
