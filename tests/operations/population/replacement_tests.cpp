
#include <replacement.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::replacement {

using fitness_cmp_t = gal::maximize<std::compare_three_way>;

struct no_tags {};

using population_t =
    gal::population<int, int, fitness_cmp_t, int, fitness_cmp_t, no_tags>;

using individual_t = population_t::individual_t;
using evaluation_t = individual_t::evaluation_t;
using offspring_t =
    gal::parentship<population_t::iterator_t, population_t::individual_t>;

template<typename Individuals, typename Fn>
auto transform_result(Individuals const& individuals, Fn&& fn) {
  auto transformed = individuals | std::views::transform(std::forward<Fn>(fn));

  using item_t = std::ranges::range_value_t<decltype(transformed)>;

  return std::vector<item_t>(std::ranges::begin(transformed),
                             std::ranges::end(transformed));
}

template<typename Individuals>
auto get_raw_fitness(Individuals const& individuals) {
  return transform_result(individuals,
                          [](auto& i) { return i.evaluation().raw(); });
}

template<typename Individuals>
auto get_scaled_fitness(Individuals const& individuals) {
  return transform_result(individuals,
                          [](auto& i) { return i.evaluation().scaled(); });
}

struct replacement_tests : public ::testing::Test {
protected:
  void SetUp() override {
    std::vector<individual_t> individuals{{0, evaluation_t{3, 7}, no_tags{}},
                                          {1, evaluation_t{4, 6}, no_tags{}},
                                          {2, evaluation_t{5, 5}, no_tags{}},
                                          {3, evaluation_t{6, 4}, no_tags{}},
                                          {4, evaluation_t{7, 3}, no_tags{}}};

    population_.insert(individuals);

    auto first = population_.individuals().begin();
    offsprings_5_ = std::vector<offspring_t>{
        offspring_t{first + 4, {9, evaluation_t{9, 0}, no_tags{}}},
        offspring_t{first + 3, {8, evaluation_t{8, 1}, no_tags{}}},
        offspring_t{first + 2, {7, evaluation_t{2, 2}, no_tags{}}},
        offspring_t{first + 1, {6, evaluation_t{1, 8}, no_tags{}}},
        offspring_t{first + 0, {5, evaluation_t{0, 9}, no_tags{}}}};

    offsprings_3_ = offsprings_5_;
    offsprings_3_.erase(offsprings_3_.begin() + 3, offsprings_3_.end());
  }

  population_t population_{fitness_cmp_t{}, fitness_cmp_t{}, true};

  std::vector<offspring_t> offsprings_5_;
  std::vector<offspring_t> offsprings_3_;

  std::mt19937 rng_;
};

TEST_F(replacement_tests, random_raw_replace_elitism_removed_count) {
  // arrange
  gal::replace::random_raw<std::mt19937, 2> op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(replaced, ::testing::SizeIs(3));
}

TEST_F(replacement_tests, random_raw_replace_elitism_removed_content) {
  // arrange
  gal::replace::random_raw<std::mt19937, 2> op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_raw_fitness(replaced),
              ::testing::UnorderedElementsAre(3, 4, 5));
}

TEST_F(replacement_tests, random_raw_replace_elitism_added_content) {
  // arrange
  gal::replace::random_raw<std::mt19937, 2> op{rng_};

  // act
  op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_raw_fitness(population_.individuals()),
              ::testing::UnorderedElementsAre(9, 8, 7, 6, 2));
}

TEST_F(replacement_tests, random_scaled_replace_elitism_removed_count) {
  // arrange
  gal::replace::random_scaled<std::mt19937, 2> op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(replaced, ::testing::SizeIs(3));
}

TEST_F(replacement_tests, random_scaled_replace_elitism_removed_content) {
  // arrange
  gal::replace::random_scaled<std::mt19937, 2> op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_scaled_fitness(replaced),
              ::testing::UnorderedElementsAre(3, 4, 5));
}

TEST_F(replacement_tests, random_scaled_replace_elitism_added_content) {
  // arrange
  gal::replace::random_scaled<std::mt19937, 2> op{rng_};

  // act
  op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_.individuals()),
              ::testing::UnorderedElementsAre(0, 1, 2, 7, 6));
}

TEST_F(replacement_tests, random_raw_replace_no_elitism_removed_count) {
  // arrange
  gal::replace::random_raw op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(replaced, ::testing::SizeIs(5));
}

TEST_F(replacement_tests, random_raw_replace_no_elitism_removed_content) {
  // arrange
  gal::replace::random_raw op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_raw_fitness(replaced),
              ::testing::UnorderedElementsAre(3, 4, 5, 6, 7));
}

TEST_F(replacement_tests, random_raw_replace_no_elitism_added_content) {
  // arrange
  gal::replace::random_raw op{rng_};

  // act
  op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_raw_fitness(population_.individuals()),
              ::testing::UnorderedElementsAre(9, 8, 2, 1, 0));
}

TEST_F(replacement_tests, random_scaled_replace_no_elitism_removed_count) {
  // arrange
  gal::replace::random_scaled op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(replaced, ::testing::SizeIs(5));
}

TEST_F(replacement_tests, random_scaled_replace_no_elitism_removed_content) {
  // arrange
  gal::replace::random_scaled op{rng_};

  // act
  auto replaced = op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_scaled_fitness(replaced),
              ::testing::UnorderedElementsAre(3, 4, 5, 6, 7));
}

TEST_F(replacement_tests, random_scaled_replace_no_elitism_added_content) {
  // arrange
  gal::replace::random_scaled op{rng_};

  // act
  op(population_, offsprings_5_);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_.individuals()),
              ::testing::UnorderedElementsAre(0, 1, 2, 8, 9));
}

} // namespace tests::replacement
