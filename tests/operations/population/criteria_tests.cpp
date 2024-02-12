
#include <criteria.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::criteria {

using fitness_cmp_t = gal::maximize<gal::max_floatingpoint_three_way>;

struct no_tags {};

using population_t = gal::population<int,
                                     double,
                                     fitness_cmp_t,
                                     gal::empty_fitness,
                                     gal::disabled_comparator,
                                     no_tags>;

using statistics_t = gal::stats::statistics<population_t,
                                            gal::stats::generation,
                                            gal::stats::extreme_fitness_raw>;

using history_t = gal::stats::history<statistics_t>;

using individual_t = population_t::individual_t;
using evaluation_t = individual_t::evaluation_t;

struct criterion_tests : public ::testing::Test {
protected:
  void SetUp() override {
    std::vector<individual_t> individuals{{0, evaluation_t{1.}, no_tags{}},
                                          {1, evaluation_t{2.}, no_tags{}}};

    population_.insert(individuals);
    history_.next(population_);
  }

  population_t population_{{}, {}, true};
  history_t history_{2};
};

TEST_F(criterion_tests, generation_limit_under) {
  // arrange
  gal::criteria::generation_limit op{2};

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_FALSE(result);
}

TEST_F(criterion_tests, generation_limit_over) {
  // arrange
  gal::criteria::generation_limit op{2};

  history_.next(population_);

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_TRUE(result);
}

TEST_F(criterion_tests, value_limit_under) {
  // arrange
  gal::criteria::value_limit op{gal::stats::get_raw_fitness_best_value{},
                                [](auto fit) { return fit >= 3.; }};

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_FALSE(result);
}

TEST_F(criterion_tests, value_limit_over) {
  // arrange
  gal::criteria::value_limit op{gal::stats::get_raw_fitness_best_value{},
                                [](auto fit) { return fit >= 3.; }};

  population_.insert(std::array{individual_t{0, evaluation_t{3.}, no_tags{}}});
  history_.next(population_);

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_TRUE(result);
}

////////

TEST_F(criterion_tests, value_progress_not_stagnated) {
  // arrange
  gal::criteria::value_progress op{
      gal::stats::get_raw_fitness_best_value{}, std::greater{}, 2};

  history_.next(population_);

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_FALSE(result);
}

TEST_F(criterion_tests, value_progress_stagnated) {
  // arrange
  gal::criteria::value_progress op{
      gal::stats::get_raw_fitness_best_value{}, std::greater{}, 2};

  history_.next(population_);

  op(population_, history_);

  // act
  auto result = op(population_, history_);

  // assert
  EXPECT_TRUE(result);
}

} // namespace tests::criteria
