
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

using tags_t = std::tuple<gal::lineage_t>;

using population_t = gal::
    population<chromosome_test, int, fitness_cmp_t, int, fitness_cmp_t, tags_t>;

using statistics_t = gal::stats::statistics<population_t, gal::stats::blank>;
using history_t = gal::stats::history<statistics_t>;

using individual_t = population_t::individual_t;
using evaluation_t = individual_t::evaluation_t;

struct scaling_test {
  inline void operator()(individual_t& individual) const {
    auto& eval = individual.eval();
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
  return transform_result(
      population, parents, [](auto& p) { return get_child(p).eval().raw(); });
}

template<typename Parents>
auto get_child_scaled_fitness(population_t const& population,
                              Parents const& parents) {
  return transform_result(population, parents, [](auto& p) {
    return get_child(p).eval().scaled();
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
        {chromosome_test{false}, evaluation_t{2, 4}, tags_t{}},
        {chromosome_test{false}, evaluation_t{2, 4}, tags_t{}},
        {chromosome_test{true}, evaluation_t{2, 4}, tags_t{}},
        {chromosome_test{true}, evaluation_t{2, 4}, tags_t{}}};

    population_.insert(individuals);

    auto first = population_.individuals().begin();
    parents_4_ = {first + 0, first + 1, first + 2, first + 3};
    parents_2_ = {first + 1, first + 2};
  }

  population_t population_{{}, {}, true};
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

  std::vector<population_t::iterator_t> parents_4_;
  std::vector<population_t::iterator_t> parents_2_;

  std::mt19937 rng_;
};

TEST_F(coupling_tests, exclusive_child_count) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(coupling_tests, exclusive_parents) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_parent_indices(population_, result),
              ::testing::ElementsAre(0, 1, 2, 3));
}

TEST_F(coupling_tests, exclusive_child_raw_fitness) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_raw_fitness(population_, result),
              ::testing::ElementsAre(2, 2, 2, 2));
}

TEST_F(coupling_tests, exclusive_child_no_scaling) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(0, 0, 0, 0));
}

TEST_F(coupling_tests, exclusive_child_scaled_fitness) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(20, 20, 20, 20));
}

TEST_F(coupling_tests, exclusive_child_no_crossover) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, exclusive_child_no_mutation) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, exclusive_child_with_crossover) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 1._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, exclusive_child_with_mutation) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 1._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, exclusive_child_with_improving_mutation) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::exclusive, 0._fc, 1._fc, true>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, true, true));
}

TEST_F(coupling_tests, overlapping_child_count) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(coupling_tests, overlapping_parents) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_parent_indices(population_, result),
              ::testing::ElementsAre(0, 1, 2, 3));
}

TEST_F(coupling_tests, overlapping_child_raw_fitness) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_raw_fitness(population_, result),
              ::testing::ElementsAre(2, 2, 2, 2));
}

TEST_F(coupling_tests, overlapping_child_no_scaling) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(0, 0, 0, 0));
}

TEST_F(coupling_tests, overlapping_child_scaled_fitness) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(20, 20, 20, 20));
}

TEST_F(coupling_tests, overlapping_child_no_crossover) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, overlapping_child_no_mutation) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, overlapping_child_with_crossover) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 1._fc, 0._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, overlapping_child_with_mutation) {
  using namespace gal::literals;

  // arrange
  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 1._fc, false>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, overlapping_child_with_improving_mutation) {
  // arrange
  using namespace gal::literals;

  auto op =
      gal::couple::parametrize<gal::couple::overlapping, 0._fc, 1._fc, true>(
          rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, true, true, true));
}

TEST_F(coupling_tests, field_child_count) {
  // arrange
  using namespace gal::literals;

  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(coupling_tests, field_parents) {
  // arrange
  using namespace gal::literals;

  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_parent_indices(population_, result),
              ::testing::ElementsAre(0, 1, 2, 3));
}

TEST_F(coupling_tests, field_child_raw_fitness) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_raw_fitness(population_, result),
              ::testing::ElementsAre(2, 2, 2, 2));
}

TEST_F(coupling_tests, field_child_no_scaling) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(0, 0, 0, 0));
}

TEST_F(coupling_tests, field_child_scaled_fitness) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(20, 20, 20, 20));
}

TEST_F(coupling_tests, field_child_no_crossover) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, field_child_no_mutation) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(false, false, false, false));
}

TEST_F(coupling_tests, field_child_with_crossover) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 1._fc, 0._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, field_child_with_mutation) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 1._fc, false>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true, true, true));
}

TEST_F(coupling_tests, field_child_with_improving_mutation) {
  using namespace gal::literals;

  // arrange
  auto op = gal::couple::parametrize<gal::couple::field, 0._fc, 1._fc, true>(
      rng_)(no_scaling_ctx_);

  // act
  auto result = op(parents_4_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true, false, false));
}

TEST_F(coupling_tests, local_child_count) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(2));
}

TEST_F(coupling_tests, local_parents) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};
  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_parent_indices(population_, result),
              ::testing::ElementsAre(1, 2));
}

TEST_F(coupling_tests, local_child_raw_fitness) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_child_raw_fitness(population_, result),
              ::testing::ElementsAre(1, 3));
}

TEST_F(coupling_tests, local_child_no_scaling) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(0, 0));
}

TEST_F(coupling_tests, local_child_scaled_fitness) {
  // arrange
  gal::couple::local op{scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_child_scaled_fitness(population_, result),
              ::testing::ElementsAre(10, 30));
}

TEST_F(coupling_tests, local_child_no_crossover) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_child_crossovers(population_, result),
              ::testing::ElementsAre(false, false));
}

TEST_F(coupling_tests, local_child_with_mutation) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  // assert
  EXPECT_THAT(get_child_mutations(population_, result),
              ::testing::ElementsAre(true, true));
}

TEST_F(coupling_tests, local_child_parent_tags) {
  // arrange
  gal::couple::local op{no_scaling_ctx_};

  // act
  auto result = op(parents_2_);

  EXPECT_EQ(gal::get_tag<gal::lineage_t>(*get_parent(result[0])).get(),
            gal::lineage::parent);
  EXPECT_EQ(gal::get_tag<gal::lineage_t>(*get_parent(result[1])).get(),
            gal::lineage::parent);

  EXPECT_EQ(gal::get_tag<gal::lineage_t>(get_child(result[0])).get(),
            gal::lineage::child);
  EXPECT_EQ(gal::get_tag<gal::lineage_t>(get_child(result[1])).get(),
            gal::lineage::child);
}

} // namespace tests::coupling
