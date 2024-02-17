
#include "..\..\random.hpp"

#include <crossover.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::crossover {

using chromosome_vector_t = std::vector<int>;
using chromosome_list_t = std::list<int>;

struct crossover_tests : public ::testing::Test {
protected:
  void SetUp() override {
    parents_vector_ = {
        chromosome_vector_t{10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
        chromosome_vector_t{20, 21, 22, 23, 24, 25, 26, 27}};

    parents_list_ = {
        chromosome_list_t{parents_vector_[0].begin(), parents_vector_[0].end()},
        chromosome_list_t{parents_vector_[1].begin(),
                          parents_vector_[1].end()}};
  }

  std::array<chromosome_vector_t, 2> parents_vector_;
  std::array<chromosome_list_t, 2> parents_list_;
};

TEST_F(crossover_tests, symmetric_singlepoint_vector) {
  // arrange
  deterministic_index_generator rng{4};
  gal::cross::symmetric_singlepoint op{rng};

  // act
  auto [child1, child2] = op(parents_vector_[0], parents_vector_[1]);

  // assert
  EXPECT_THAT(child1, ::testing::ElementsAre(10, 11, 12, 13, 24, 25, 26, 27));

  EXPECT_THAT(child2,
              ::testing::ElementsAre(20, 21, 22, 23, 14, 15, 16, 17, 18, 19));
}

TEST_F(crossover_tests, symmetric_singlepoint_list) {
  // arrange
  deterministic_index_generator rng{4};
  gal::cross::symmetric_singlepoint op{rng};

  // act
  auto [child1, child2] = op(parents_list_[0], parents_list_[1]);

  // assert
  EXPECT_THAT(child1, ::testing::ElementsAre(10, 11, 12, 13, 24, 25, 26, 27));

  EXPECT_THAT(child2,
              ::testing::ElementsAre(20, 21, 22, 23, 14, 15, 16, 17, 18, 19));
}

TEST_F(crossover_tests, asymmetric_singlepoint_vector) {
  // arrange
  deterministic_index_generator rng{3, 5};
  gal::cross::asymmetric_singlepoint op{rng};

  // act
  auto [child1, child2] = op(parents_vector_[0], parents_vector_[1]);

  // assert
  EXPECT_THAT(child1, ::testing::ElementsAre(10, 11, 12, 25, 26, 27));

  EXPECT_THAT(
      child2,
      ::testing::ElementsAre(20, 21, 22, 23, 24, 13, 14, 15, 16, 17, 18, 19));
}

TEST_F(crossover_tests, asymmetric_singlepoint_list) {
  // arrange
  deterministic_index_generator rng{3, 5};
  gal::cross::asymmetric_singlepoint op{rng};

  // act
  auto [child1, child2] = op(parents_list_[0], parents_list_[1]);

  // assert
  EXPECT_THAT(child1, ::testing::ElementsAre(10, 11, 12, 25, 26, 27));

  EXPECT_THAT(
      child2,
      ::testing::ElementsAre(20, 21, 22, 23, 24, 13, 14, 15, 16, 17, 18, 19));
}

TEST_F(crossover_tests, symmetric_multipoint_vector) {
  // arrange
  deterministic_index_generator rng{3, 6};
  gal::cross::symmetric_multipoint op{rng, gal::countable<2>};

  // act
  auto [child1, child2] = op(parents_vector_[0], parents_vector_[1]);

  // assert
  EXPECT_THAT(child1,
              ::testing::ElementsAre(10, 11, 12, 23, 24, 25, 16, 17, 18, 19));

  EXPECT_THAT(child2, ::testing::ElementsAre(20, 21, 22, 13, 14, 15, 26, 27));
}

TEST_F(crossover_tests, symmetric_multipoint_list) {
  // arrange
  deterministic_index_generator rng{3, 6};
  gal::cross::symmetric_multipoint op{rng, gal::countable<2>};

  // act
  auto [child1, child2] = op(parents_list_[0], parents_list_[1]);

  // assert
  EXPECT_THAT(child1,
              ::testing::ElementsAre(10, 11, 12, 23, 24, 25, 16, 17, 18, 19));

  EXPECT_THAT(child2, ::testing::ElementsAre(20, 21, 22, 13, 14, 15, 26, 27));
}

TEST_F(crossover_tests, asymmetric_multipoint_vector) {
  // arrange
  deterministic_index_generator rng{3, 6, 1, 7};
  gal::cross::asymmetric_multipoint op{rng, gal::countable<2>};

  // act
  auto [child1, child2] = op(parents_vector_[0], parents_vector_[1]);

  // assert
  EXPECT_THAT(child1,
              ::testing::ElementsAre(
                  10, 11, 12, 21, 22, 23, 24, 25, 26, 16, 17, 18, 19));

  EXPECT_THAT(child2, ::testing::ElementsAre(20, 13, 14, 15, 27));
}

TEST_F(crossover_tests, asymmetric_multipoint_list) {
  // arrange
  deterministic_index_generator rng{3, 6, 1, 7};
  gal::cross::asymmetric_multipoint op{rng, gal::countable<2>};

  // act
  auto [child1, child2] = op(parents_list_[0], parents_list_[1]);

  // assert
  EXPECT_THAT(child1,
              ::testing::ElementsAre(
                  10, 11, 12, 21, 22, 23, 24, 25, 26, 16, 17, 18, 19));

  EXPECT_THAT(child2, ::testing::ElementsAre(20, 13, 14, 15, 27));
}

TEST_F(crossover_tests, blend_vector) {
  // arrange
  gal::cross::blend op{[](auto& l, auto& r) { return std::pair{r, l}; }};

  // act
  auto [child1, child2] = op(parents_vector_[0], parents_vector_[1]);

  // assert
  EXPECT_THAT(child1,
              ::testing::ElementsAre(20, 21, 22, 23, 24, 25, 26, 27, 18, 19));

  EXPECT_THAT(child2, ::testing::ElementsAre(10, 11, 12, 13, 14, 15, 16, 17));
}

} // namespace tests::crossover
