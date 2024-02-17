
#include "..\..\random.hpp"

#include <mutation.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::mutation {

using chromosome_vector_t = std::vector<int>;
using chromosome_list_t = std::list<int>;

struct mutation_tests : public ::testing::Test {
protected:
  void SetUp() override {
    chromosome_vector_ = chromosome_vector_t{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    chromosome_list_ =
        chromosome_list_t{chromosome_vector_.begin(), chromosome_vector_.end()};
  }

  chromosome_vector_t chromosome_vector_;
  chromosome_list_t chromosome_list_;
};

TEST_F(mutation_tests, interchange_vector_one_pair) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::interchange op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 8, 2, 3, 4, 5, 6, 7, 1, 9));
}

TEST_F(mutation_tests, interchange_vector_two_pairs) {
  // arrange
  deterministic_index_generator rng_{1, 8, 0, 9};
  gal::mutate::interchange op{rng_, gal::countable<2>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(9, 8, 2, 3, 4, 5, 6, 7, 1, 0));
}

TEST_F(mutation_tests, interchange_list_one_pair) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::interchange op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 8, 2, 3, 4, 5, 6, 7, 1, 9));
}

TEST_F(mutation_tests, interchange_list_two_pairs) {
  // arrange
  deterministic_index_generator rng_{1, 8, 0, 9};
  gal::mutate::interchange op{rng_, gal::countable<2>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(9, 8, 2, 3, 4, 5, 6, 7, 1, 0));
}

TEST_F(mutation_tests, suffle_vector_one_forward) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 1, 9));
}

TEST_F(mutation_tests, suffle_vector_one_backward) {
  // arrange
  deterministic_index_generator rng{8, 1};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 8, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, suffle_vector_two) {
  // arrange
  deterministic_index_generator rng{0, 9, 8, 0};
  gal::mutate::shuffle op{rng, gal::countable<2>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(9, 1, 2, 3, 4, 5, 6, 7, 8, 0));
}

TEST_F(mutation_tests, suffle_list_one_forward) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 1, 9));
}

TEST_F(mutation_tests, suffle_list_one_backward) {
  // arrange
  deterministic_index_generator rng{8, 1};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 8, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, suffle_list_two) {
  // arrange
  deterministic_index_generator rng{0, 9, 8, 0};
  gal::mutate::shuffle op{rng, gal::countable<2>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(9, 1, 2, 3, 4, 5, 6, 7, 8, 0));
}

TEST_F(mutation_tests, destroy_vector_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::destroy op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, destroy_vector_two) {
  // arrange
  deterministic_index_generator rng{0, 8};
  gal::mutate::destroy op{rng, gal::countable<2>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8));
}

TEST_F(mutation_tests, destroy_list_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::destroy op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, destroy_list_two) {
  // arrange
  deterministic_index_generator rng{0, 8};
  gal::mutate::destroy op{rng, gal::countable<2>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_, ::testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8));
}

TEST_F(mutation_tests, create_vector_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::create op{rng, gal::countable<1>, []() { return 10; }};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 10, 8, 9));
}

TEST_F(mutation_tests, create_vector_two) {
  // arrange
  deterministic_index_generator rng{0, 11};
  gal::mutate::create op{rng, gal::countable<2>, []() { return 10; }};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST_F(mutation_tests, create_list_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::create op{rng, gal::countable<1>, []() { return 10; }};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 10, 8, 9));
}

TEST_F(mutation_tests, create_list_two) {
  // arrange
  deterministic_index_generator rng{0, 11};
  gal::mutate::create op{rng, gal::countable<2>, []() { return 10; }};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST_F(mutation_tests, flip_vector_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::flip op{rng, gal::countable<1>, [](auto& x) { x = 10; }};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 10, 9));
}

TEST_F(mutation_tests, flip_vector_two) {
  // arrange
  deterministic_index_generator rng{0, 9};
  gal::mutate::flip op{rng, gal::countable<2>, [](auto& x) { x = 10; }};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(10, 1, 2, 3, 4, 5, 6, 7, 8, 10));
}

TEST_F(mutation_tests, flip_list_one) {
  // arrange
  deterministic_index_generator rng{8};
  gal::mutate::flip op{rng, gal::countable<1>, [](auto& x) { x = 10; }};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 10, 9));
}

TEST_F(mutation_tests, flip_list_two) {
  // arrange
  deterministic_index_generator rng{0, 9};
  gal::mutate::flip op{rng, gal::countable<2>, [](auto& x) { x = 10; }};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(10, 1, 2, 3, 4, 5, 6, 7, 8, 10));
}

} // namespace tests::mutation
