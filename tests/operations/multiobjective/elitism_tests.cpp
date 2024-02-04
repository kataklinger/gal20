
#include "elitism.hpp"

#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::elitism {

using raw_cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using population_t =
    gal::population<int,
                    fitness_t,
                    raw_cmp_t,
                    gal::empty_fitness,
                    gal::disabled_comparator,
                    std::tuple<gal::frontier_level_t, gal::int_rank_t>>;

constexpr fitness_t f1a{0, 0};
constexpr fitness_t f2a{1, 0};
constexpr fitness_t f2b{0, 1};
constexpr fitness_t f3a{1, 1};

class elitism_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{f1a}},
                                          {0, evaluation_t{f2a}},
                                          {0, evaluation_t{f2b}},
                                          {0, evaluation_t{f3a}}};

    population_4_.insert(individuals);
  }

  population_t population_4_{raw_cmp_t{}, gal::disabled_comparator{}, false};
};

TEST_F(elitism_tests, strict_preserved_population_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_preserved_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(population_4_.current_size(), 1);
  EXPECT_EQ(population_4_.individuals()[0].evaluation().raw(), f1a);
}

TEST_F(elitism_tests, strict_preserved_fronts_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_preserved_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(pareto.size(), 1);
  EXPECT_EQ(pareto.get_size_of(1), 1);
  EXPECT_EQ((*pareto.begin()->begin())->evaluation().raw(), f1a);
}

TEST_F(elitism_tests, strict_reduced_population_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_reduced_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(population_4_.current_size(), 1);
  EXPECT_EQ(population_4_.individuals()[0].evaluation().raw(), f1a);
}

TEST_F(elitism_tests, strict_reduced_fronts_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_reduced_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(pareto.size(), 1);
  EXPECT_EQ(pareto.get_size_of(1), 1);
  EXPECT_EQ((*pareto.begin()->begin())->evaluation().raw(), f1a);
}

TEST_F(elitism_tests, strict_nondominated_population_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_nondominated_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(population_4_.current_size(), 1);
  EXPECT_EQ(population_4_.individuals()[0].evaluation().raw(), f1a);
}

TEST_F(elitism_tests, strict_ondominated_fronts_content) {
  // arrange
  auto pareto = gal::rank::level{}(population_4_, gal::pareto_nondominated_tag);

  // act
  gal::elite::strict{}(population_4_, pareto);

  // assert
  EXPECT_EQ(pareto.size(), 1);
  EXPECT_EQ(pareto.get_size_of(1), 1);
  EXPECT_EQ((*pareto.begin()->begin())->evaluation().raw(), f1a);
}

} // namespace tests::elitism
