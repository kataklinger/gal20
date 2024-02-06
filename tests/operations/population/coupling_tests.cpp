
#include <context.hpp>
#include <coupling.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::coupling {

struct chromosome_test {
  bool improve_{};
  bool crossedover_{};
  bool mutated_{};

  bool operator==(chromosome_test const&) const = default;
};

struct crossover_test {
  inline auto operator()(chromosome_test const& left,
                         chromosome_test const& right) const noexcept {
    return std::pair{chromosome_test{left.improve_, true, false},
                     chromosome_test{right.improve_, true, false}};
  }
};

struct mutation_test {
  inline void operator()(chromosome_test& c) const noexcept {
    c.mutated_ = true;
  }
};

struct evaluation_test {
  inline auto operator()(chromosome_test const& c) const noexcept {
    return !c.mutated_ ? 2 : c.improve_ ? 3 : 1;
  }
};

using fitness_cmp_t = gal::maximize<std::compare_three_way>;

struct tags {};

using population_t = gal::
    population<chromosome_test, int, fitness_cmp_t, int, fitness_cmp_t, tags>;

using statistics_t = gal::stats::statistics<population_t, gal::stats::blank>;
using history_t = gal::stats::history<statistics_t>;

using individual_t = population_t::individual_t;
using evaluation_t = individual_t::evaluation_t;

struct scaling_test {
  inline void operator()(individual_t& individual) const {
    auto& eval = individual.evaluation();
    eval.set_scaled(eval.raw() * 10);
  }
};

template<typename Parents, typename Fn>
auto transform_result(population_t const& population,
                      Parents const& parents,
                      Fn&& fn) {
  auto transformed = parents | std::views::transform(std::forward<Fn>(fn));

  using item_t = std::ranges::range_value_t<decltype(transformed)>;

  return std::vector<item_t>(std::ranges::begin(transformed),
                             std::ranges::end(transformed));
}

template<typename Parents>
auto get_parent_indices(population_t const& population,
                        Parents const& parents) {
  auto first = population.individuals().begin();
  return transform_result(
      population, parents, [first](auto& p) { return get_parent(p) - first; });
}

template<typename Parents>
auto get_child_raw_fitness(population_t const& population,
                           Parents const& parents) {
  return transform_result(population, parents, [](auto& p) {
    return get_child(p).evaluation().raw();
  });
}

template<typename Parents>
auto get_child_scaled_fitness(population_t const& population,
                              Parents const& parents) {
  return transform_result(population, parents, [](auto& p) {
    return get_child(p).evaluation().scaled();
  });
}

template<typename Parents>
auto get_child_crossovers(population_t const& population,
                          Parents const& parents) {
  return transform_result(population, parents, [](auto& p) {
    return get_child(p).chromosome().crossedover_;
  });
}

template<typename Parents>
auto get_child_mutations(population_t const& population,
                         Parents const& parents) {
  return transform_result(population, parents, [](auto& p) {
    return get_child(p).chromosome().mutated_;
  });
}

struct coupling_tests : public ::testing::Test {
protected:
  void SetUp() override {
    std::vector<individual_t> individuals{
        {chromosome_test{false}, evaluation_t{2, 4}, tags{}},
        {chromosome_test{false}, evaluation_t{2, 4}, tags{}},
        {chromosome_test{true}, evaluation_t{2, 4}, tags{}},
        {chromosome_test{true}, evaluation_t{2, 4}, tags{}}};

    population_.insert(individuals);

    auto first = population_.individuals().begin();
    parents_ = {first + 0, first + 1, first + 2, first + 3};
  }

  population_t population_{fitness_cmp_t{}, fitness_cmp_t{}, true};
  history_t history_{1};

  gal::reproduction_context<population_t,
                            statistics_t,
                            crossover_test,
                            mutation_test,
                            evaluation_test>
      no_scaling_ctx_{population_, history_, {}, {}, {}};

  gal::reproduction_context_with_scaling<population_t,
                                         statistics_t,
                                         crossover_test,
                                         mutation_test,
                                         evaluation_test,
                                         scaling_test>
      scaling_ctx_{population_, history_, {}, {}, {}, {}};

  std::vector<population_t::iterator_t> parents_;

  std::mt19937 rng_;
};

TEST_F(coupling_tests, exclusive_child_count) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(coupling_tests, exclusive_parents) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_parent_indices(population_, result),
              ::testing::ElementsAre(0, 1, 2, 3));
}

TEST_F(coupling_tests, exclusive_child_raw_fitness) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_raw_fitness(population_, result),
              ::testing::ElementsAre(2, 2, 2, 2));
}

TEST_F(coupling_tests, exclusive_child_no_scaling) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(0, 0, 0, 0));
}

TEST_F(coupling_tests, exclusive_child_scaled_fitness) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(20, 20, 20, 20));
}

TEST_F(coupling_tests, exclusive_child_no_crossover) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, exclusive_child_no_mutation) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, exclusive_child_with_crossover) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 1., 0., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, exclusive_child_with_mutation) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 1., false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, exclusive_child_with_improving_mutation) {
  // arrange
  auto op = gal::couple::parametrize<gal::couple::exclusive, 0., 1., true>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, true, true));
}

} // namespace tests::coupling
