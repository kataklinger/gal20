
#include <selection.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::selection {

using fitness_t = double;

using cmp_t = gal::minimize<gal::floatingpoint_three_way>;

struct no_tags {};

using population_t =
    gal::population<int, fitness_t, cmp_t, fitness_t, cmp_t, no_tags>;

class random_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{0.1, 0.1}},
                                          {0, evaluation_t{0.2, 0.2}},
                                          {0, evaluation_t{0.3, 0.3}},
                                          {0, evaluation_t{0.4, 0.4}},
                                          {0, evaluation_t{0.5, 0.5}},
                                          {0, evaluation_t{0.6, 0.6}},
                                          {0, evaluation_t{0.7, 0.7}},
                                          {0, evaluation_t{0.8, 0.8}},
                                          {0, evaluation_t{0.9, 0.9}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, cmp_t{}, true};
  std::mt19937 rng_{};
};

TEST_F(random_tests, unique_selection_size) {
  // arrange
  gal::select::random op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(random_tests, unique_selection_content) {
  // arrange
  gal::select::random op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  auto transformed = result | std::views::transform([](auto const& i) {
                       return i->evaluation().raw();
                     });

  std::unordered_set<double> unique{transformed.begin(), transformed.end()};
  EXPECT_THAT(unique, ::testing::SizeIs(8));
}

TEST_F(random_tests, nonunique_selection_size) {
  // arrange
  gal::select::random op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(random_tests, nonunique_selection_content) {
  // arrange
  gal::select::random op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  auto transformed = result | std::views::transform([](auto const& i) {
                       return i->evaluation().raw();
                     });

  std::unordered_set<double> unique{transformed.begin(), transformed.end()};
  EXPECT_THAT(unique.size(), ::testing::Le(8));
}

} // namespace tests::selection
