
#include <selection.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::selection {

template<typename Results>
inline auto get_transformed(Results& results) {
  return results | std::views::transform(
                       [](auto const& i) { return i->evaluation().raw(); });
};

template<typename Results>
inline auto get_unique(Results& results) {
  auto transformed = get_transformed(results);
  return std::vector<double>{transformed.begin(), transformed.end()};
};

template<typename Results>
inline auto get_unique_count(Results& results) {
  auto transformed = get_transformed(results);
  auto unique =
      std::unordered_set<double>{transformed.begin(), transformed.end()};
  return unique.size();
};

class selection_tests : public ::testing::Test {
protected:
  using fitness_t = double;

  using cmp_t = gal::maximize<gal::floatingpoint_three_way>;

  struct no_tags {};

  using population_t =
      gal::population<int, fitness_t, cmp_t, fitness_t, cmp_t, no_tags>;

  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{7., 7.}},
                                          {0, evaluation_t{6., 6.}},
                                          {0, evaluation_t{4., 4.}},
                                          {0, evaluation_t{8., 8.}},
                                          {0, evaluation_t{9., 9.}},
                                          {0, evaluation_t{3., 3.}},
                                          {0, evaluation_t{2., 2.}},
                                          {0, evaluation_t{1., 1.}},
                                          {0, evaluation_t{5., 5.}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, cmp_t{}, true};
  std::mt19937 rng_{};
};

TEST_F(selection_tests, random_unique_selection_size) {
  // arrange
  gal::select::random op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, random_unique_selection_content) {
  // arrange
  gal::select::random op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(selection_tests, random_nonunique_selection_size) {
  // arrange
  gal::select::random op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, random_nonunique_selection_content) {
  // arrange
  gal::select::random op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

TEST_F(selection_tests, tournament_raw_unique_selection_size) {
  // arrange
  gal::select::tournament_raw op{
      gal::select::unique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, tournament_raw_unique_selection_content) {
  // arrange
  gal::select::tournament_raw op{
      gal::select::unique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(selection_tests, tournament_raw_nonunique_selection_size) {
  // arrange
  gal::select::tournament_raw op{
      gal::select::nonunique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, tournament_raw_nonunique_selection_content) {
  // arrange
  gal::select::tournament_raw op{
      gal::select::nonunique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

TEST_F(selection_tests, tournament_scaled_unique_selection_size) {
  // arrange
  gal::select::tournament_scaled op{
      gal::select::unique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, tournament_scaled_unique_selection_content) {
  // arrange
  gal::select::tournament_scaled op{
      gal::select::unique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(selection_tests, tournament_scaled_nonunique_selection_size) {
  // arrange
  gal::select::tournament_scaled op{
      gal::select::nonunique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, tournament_scaled_nonunique_selection_content) {
  // arrange
  gal::select::tournament_scaled op{
      gal::select::nonunique<8>, gal::select::rounds<2>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

TEST_F(selection_tests, best_raw_selection_size) {
  // arrange
  gal::select::best_raw<4> op{};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(selection_tests, best_raw_selection_content) {
  // arrange
  gal::select::best_raw<4> op{};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique(result), ::testing::ElementsAre(9., 8., 7., 6.));
}

TEST_F(selection_tests, best_scaled_selection_size) {
  // arrange
  gal::select::best_scaled<4> op{};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(4));
}

TEST_F(selection_tests, best_scaled_selection_content) {
  // arrange
  gal::select::best_scaled<4> op{};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique(result), ::testing::ElementsAre(9., 8., 7., 6.));
}

TEST_F(selection_tests, roulette_raw_unique_selection_size) {
  // arrange
  gal::select::roulette_raw op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, roulette_raw_unique_selection_content) {
  // arrange
  gal::select::roulette_raw op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(selection_tests, roulette_raw_nonunique_selection_size) {
  // arrange
  gal::select::roulette_raw op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, roulette_raw_nonunique_selection_content) {
  // arrange
  gal::select::roulette_raw op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

TEST_F(selection_tests, roulette_scaled_unique_selection_size) {
  // arrange
  gal::select::roulette_scaled op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, roulette_scaled_unique_selection_content) {
  // arrange
  gal::select::roulette_scaled op{gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(selection_tests, roulette_scaled_nonunique_selection_size) {
  // arrange
  gal::select::roulette_scaled op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(selection_tests, roulette_scaled_nonunique_selection_content) {
  // arrange
  gal::select::roulette_scaled op{gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

class cluster_selection_tests : public ::testing::Test {
protected:
  using fitness_t = double;

  using cmp_t = gal::maximize<gal::floatingpoint_three_way>;

  using tags_t = gal::cluster_label;

  using population_t =
      gal::population<int, fitness_t, cmp_t, fitness_t, cmp_t, tags_t>;

  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{7., 7.}, tags_t{0}},
        {0, evaluation_t{6., 6.}, tags_t{gal::cluster_label::unassigned()}},
        {0, evaluation_t{6., 6.}, tags_t{gal::cluster_label::unique()}},
        {0, evaluation_t{4., 4.}, tags_t{0}},
        {0, evaluation_t{8., 8.}, tags_t{1}},
        {0, evaluation_t{9., 9.}, tags_t{1}},
        {0, evaluation_t{3., 3.}, tags_t{4}},
        {0, evaluation_t{2., 2.}, tags_t{3}},
        {0, evaluation_t{1., 1.}, tags_t{gal::cluster_label::unique()}},
        {0, evaluation_t{5., 5.}, tags_t{0}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, cmp_t{}, true};
  std::mt19937 rng_{};
};

TEST_F(cluster_selection_tests, uniform_unique_selection_size) {
  // arrange
  gal::select::cluster op{
      gal::select::uniform<gal::cluster_label>, gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(cluster_selection_tests, uniform_unique_selection_content) {
  // arrange
  gal::select::cluster op{
      gal::select::uniform<gal::cluster_label>, gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(cluster_selection_tests, uniform_nonunique_selection_size) {
  // arrange
  gal::select::cluster op{gal::select::uniform<gal::cluster_label>,
                          gal::select::nonunique<8>,
                          rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(cluster_selection_tests, uniform_nonunique_selection_content) {
  // arrange
  gal::select::cluster op{gal::select::uniform<gal::cluster_label>,
                          gal::select::nonunique<8>,
                          rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

////

TEST_F(cluster_selection_tests, shared_unique_selection_size) {
  // arrange
  gal::select::cluster op{
      gal::select::shared<gal::cluster_label>, gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(cluster_selection_tests, shared_unique_selection_content) {
  // arrange
  gal::select::cluster op{
      gal::select::shared<gal::cluster_label>, gal::select::unique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Eq(8));
}

TEST_F(cluster_selection_tests, shared_nonunique_selection_size) {
  // arrange
  gal::select::cluster op{
      gal::select::shared<gal::cluster_label>, gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(result, ::testing::SizeIs(8));
}

TEST_F(cluster_selection_tests, shared_nonunique_selection_content) {
  // arrange
  gal::select::cluster op{
      gal::select::shared<gal::cluster_label>, gal::select::nonunique<8>, rng_};

  // act
  auto result = op(population_);

  // assert
  EXPECT_THAT(get_unique_count(result), ::testing::Le(8));
}

} // namespace tests::selection
