

#include "pruning.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::pruning {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t = std::tuple<gal::frontier_level_t,
                          gal::int_rank_t,
                          gal::crowd_density_t,
                          gal::cluster_label,
                          gal::prune_state_t>;

using population_t = gal::
    population<int, fitness_t, cmp_t, double, gal::disabled_comparator, tags_t>;

template<typename Tag, typename Population>
inline auto get_tag_value(Population const& population, std::size_t index) {
  return gal::get_tag<Tag>(population.individuals()[index]);
}

template<typename Population>
inline auto get_crowd_density(Population const& population, std::size_t index) {
  return get_tag_value<gal::crowd_density_t>(population, index).get();
}

template<typename Population>
inline auto get_ranking(Population const& population, std::size_t index) {
  return get_tag_value<gal::int_rank_t>(population, index).get();
}

template<typename Population>
inline auto get_cluster(Population const& population, std::size_t index) {
  return get_tag_value<gal::cluster_label>(population, index).index();
}

class global_worst_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {0, evaluation_t{fit}, tags_t{3, 3, 0.1, 0, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.4, 0, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.3, 0, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.2, 0, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.1, 0, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.3, 0, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.2, 0, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.1, 0, false}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, 5, false};
};

TEST_F(global_worst_tests, overflowed_population_size) {
  // arrange
  gal::prune::global_worst<gal::int_rank_t> op{};

  // act
  op(population_);

  // assert
  EXPECT_EQ(population_.current_size(), 5);
}

TEST_F(global_worst_tests, overflowed_population_content) {
  // arrange
  gal::prune::global_worst<gal::int_rank_t> op{};

  // act
  op(population_);

  // assert
  EXPECT_EQ(get_ranking(population_, 0), 1);
  EXPECT_NEAR(get_crowd_density(population_, 0), 0.1, 0.01);

  EXPECT_EQ(get_ranking(population_, 1), 1);
  EXPECT_NEAR(get_crowd_density(population_, 1), 0.2, 0.01);

  EXPECT_EQ(get_ranking(population_, 2), 1);
  EXPECT_NEAR(get_crowd_density(population_, 2), 0.3, 0.01);

  EXPECT_EQ(get_ranking(population_, 3), 2);
  EXPECT_NEAR(get_crowd_density(population_, 3), 0.1, 0.01);

  EXPECT_EQ(get_ranking(population_, 4), 2);
  EXPECT_NEAR(get_crowd_density(population_, 4), 0.2, 0.01);
}

class cluster_random_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 0, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 0, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 0, false}},

        {0, evaluation_t{fit}, tags_t{1, 1, 0, 1, false}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 1, false}},

        {0, evaluation_t{fit}, tags_t{1, 1, 0, 2, false}},

        {0, evaluation_t{fit}, tags_t{2, 2, 0, 3, false}},

        {0, evaluation_t{fit}, tags_t{2, 2, 0, 4, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 4, false}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 4, false}},

        {0, evaluation_t{fit}, tags_t{3, 3, 0, 5, false}},

        {0, evaluation_t{fit}, tags_t{3, 3, 0, 6, false}},
        {0, evaluation_t{fit}, tags_t{3, 3, 0, 6, false}}};

    clusters_.next_level();
    clusters_.add_cluster(3);
    clusters_.add_cluster(2);
    clusters_.add_cluster(1);

    clusters_.next_level();
    clusters_.add_cluster(1);
    clusters_.add_cluster(3);

    clusters_.next_level();
    clusters_.add_cluster(1);
    clusters_.add_cluster(2);

    population_3_.insert(individuals);
    population_5_.insert(individuals);
    population_6_.insert(individuals);
    population_9_.insert(individuals);
    population_12_.insert(individuals);
    population_13_.insert(individuals);
  }

  std::mt19937 rng_{};

  population_t population_3_{cmp_t{}, gal::disabled_comparator{}, 3, false};
  population_t population_5_{cmp_t{}, gal::disabled_comparator{}, 5, false};
  population_t population_6_{cmp_t{}, gal::disabled_comparator{}, 6, false};
  population_t population_9_{cmp_t{}, gal::disabled_comparator{}, 9, false};
  population_t population_12_{cmp_t{}, gal::disabled_comparator{}, 12, false};
  population_t population_13_{cmp_t{}, gal::disabled_comparator{}, 13, false};

  gal::cluster_set clusters_;
};

TEST_F(cluster_random_tests, no_pruning_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_13_, clusters_);

  // act
  EXPECT_EQ(population_13_.current_size(), 13);
}

TEST_F(cluster_random_tests, pruning_one_cluster_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_12_, clusters_);

  // act
  EXPECT_EQ(population_12_.current_size(), 12);
}

TEST_F(cluster_random_tests, pruning_one_cluster_population_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_12_, clusters_);

  // assert
  EXPECT_EQ(get_ranking(population_12_, 10), 3);
  EXPECT_EQ(get_cluster(population_12_, 10), 5);

  EXPECT_EQ(get_ranking(population_12_, 11), 3);
  EXPECT_EQ(get_cluster(population_12_, 11), 6);
}

TEST_F(cluster_random_tests, pruning_one_cluster_set_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_12_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 3}, {1, 2}, {1, 1}, {2, 1}, {2, 3}, {3, 1}, {3, 1}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

TEST_F(cluster_random_tests,
       pruning_one_level_and_one_cluster_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_9_, clusters_);

  // act
  EXPECT_EQ(population_9_.current_size(), 9);
}

TEST_F(cluster_random_tests,
       pruning_one_level_and_one_cluster_population_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_9_, clusters_);

  // assert
  EXPECT_EQ(get_ranking(population_9_, 6), 2);
  EXPECT_EQ(get_cluster(population_9_, 6), 3);

  EXPECT_EQ(get_ranking(population_9_, 7), 2);
  EXPECT_EQ(get_cluster(population_9_, 7), 4);

  EXPECT_EQ(get_ranking(population_9_, 8), 2);
  EXPECT_EQ(get_cluster(population_9_, 8), 4);
}

TEST_F(cluster_random_tests, pruning_one_level_and_one_cluster_set_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_9_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 3}, {1, 2}, {1, 1}, {2, 1}, {2, 2}, {3, 0}, {3, 0}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

TEST_F(cluster_random_tests, pruning_multiple_levels_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_6_, clusters_);

  // act
  EXPECT_EQ(population_6_.current_size(), 6);
}

TEST_F(cluster_random_tests, pruning_multiple_levels_population_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_6_, clusters_);

  // assert
  EXPECT_EQ(get_ranking(population_6_, 2), 1);
  EXPECT_EQ(get_cluster(population_6_, 2), 0);

  EXPECT_EQ(get_ranking(population_6_, 3), 1);
  EXPECT_EQ(get_cluster(population_6_, 3), 1);

  EXPECT_EQ(get_ranking(population_6_, 4), 1);
  EXPECT_EQ(get_cluster(population_6_, 4), 1);

  EXPECT_EQ(get_ranking(population_6_, 5), 1);
  EXPECT_EQ(get_cluster(population_6_, 5), 2);
}

TEST_F(cluster_random_tests, pruning_multiple_levels_set_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_6_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 3}, {1, 2}, {1, 1}, {2, 0}, {2, 0}, {3, 0}, {3, 0}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

TEST_F(cluster_random_tests, pruning_nondominated_level_some_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_5_, clusters_);

  // act
  EXPECT_EQ(population_5_.current_size(), 5);
}

TEST_F(cluster_random_tests,
       pruning_nondominated_level_some_population_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_5_, clusters_);

  // assert
  EXPECT_EQ(get_ranking(population_5_, 1), 1);
  EXPECT_EQ(get_cluster(population_5_, 1), 0);

  EXPECT_EQ(get_ranking(population_5_, 2), 1);
  EXPECT_EQ(get_cluster(population_5_, 2), 1);

  EXPECT_EQ(get_ranking(population_5_, 3), 1);
  EXPECT_EQ(get_cluster(population_5_, 3), 1);

  EXPECT_EQ(get_ranking(population_5_, 4), 1);
  EXPECT_EQ(get_cluster(population_5_, 4), 2);
}

TEST_F(cluster_random_tests, pruning_nondominated_level_some_set_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_5_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 2}, {1, 2}, {1, 1}, {2, 0}, {2, 0}, {3, 0}, {3, 0}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

TEST_F(cluster_random_tests, pruning_nondominated_level_all_population_size) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // assert
  op(population_3_, clusters_);

  // act
  EXPECT_EQ(population_3_.current_size(), 3);
}

TEST_F(cluster_random_tests,
       pruning_nondominated_level_all_population_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_3_, clusters_);

  // assert
  EXPECT_EQ(get_ranking(population_3_, 0), 1);
  EXPECT_EQ(get_cluster(population_3_, 0), 0);

  EXPECT_EQ(get_ranking(population_3_, 1), 1);
  EXPECT_EQ(get_cluster(population_3_, 1), 1);

  EXPECT_EQ(get_ranking(population_3_, 2), 1);
  EXPECT_EQ(get_cluster(population_3_, 2), 2);
}

TEST_F(cluster_random_tests, pruning_nondominated_level_all_set_content) {
  // arrange
  gal::prune::cluster_random op{rng_};

  // act
  op(population_3_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 1}, {1, 1}, {1, 1}, {2, 0}, {2, 0}, {3, 0}, {3, 0}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

constexpr fitness_t f1a{0.1, 1.9};
constexpr fitness_t f1b{0.2, 1.8};
constexpr fitness_t f1c{0.3, 1.7};
constexpr fitness_t f1d{0.8, 1.2};
constexpr fitness_t f1e{1.2, 0.8};
constexpr fitness_t f1f{1.7, 0.3};
constexpr fitness_t f1g{1.8, 0.2};
constexpr fitness_t f1h{1.9, 0.1};

constexpr fitness_t f2a{1.1, 2.9};
constexpr fitness_t f2b{1.2, 2.8};
constexpr fitness_t f2c{1.3, 2.7};
constexpr fitness_t f2d{1.8, 2.2};
constexpr fitness_t f2e{2.2, 1.8};
constexpr fitness_t f2f{2.7, 1.3};
constexpr fitness_t f2g{2.8, 1.2};
constexpr fitness_t f2h{2.9, 1.1};

class cluster_edge_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{1, 1, 0, 0, false}},
        {0, evaluation_t{f1b}, tags_t{1, 1, 0, 0, false}},
        {0, evaluation_t{f1c}, tags_t{1, 1, 0, 0, false}},

        {0, evaluation_t{f1d}, tags_t{1, 1, 0, 1, false}},

        {0, evaluation_t{f1e}, tags_t{1, 1, 0, 2, false}},

        {0, evaluation_t{f1f}, tags_t{1, 1, 0, 3, false}},
        {0, evaluation_t{f1g}, tags_t{1, 1, 0, 3, false}},
        {0, evaluation_t{f1h}, tags_t{1, 1, 0, 3, false}},

        {0, evaluation_t{f2a}, tags_t{2, 2, 0, 4, false}},
        {0, evaluation_t{f2b}, tags_t{2, 2, 0, 4, false}},
        {0, evaluation_t{f2c}, tags_t{2, 2, 0, 4, false}},

        {0, evaluation_t{f2d}, tags_t{2, 2, 0, 5, false}},

        {0, evaluation_t{f2e}, tags_t{2, 2, 0, 6, false}},

        {0, evaluation_t{f2f}, tags_t{2, 2, 0, 7, false}},
        {0, evaluation_t{f2g}, tags_t{2, 2, 0, 7, false}},
        {0, evaluation_t{f2h}, tags_t{2, 2, 0, 7, false}}};

    clusters_.next_level();
    clusters_.add_cluster(3);
    clusters_.add_cluster(1);
    clusters_.add_cluster(1);
    clusters_.add_cluster(3);

    clusters_.next_level();
    clusters_.add_cluster(3);
    clusters_.add_cluster(1);
    clusters_.add_cluster(1);
    clusters_.add_cluster(3);

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};

  gal::cluster_set clusters_;
};

TEST_F(cluster_edge_tests, pruning_population_size) {
  // arrange
  gal::prune::cluster_edge op{};

  // assert
  op(population_, clusters_);

  // act
  EXPECT_EQ(population_.current_size(), 8);
}

TEST_F(cluster_edge_tests, pruning_population_content) {
  // arrange
  gal::prune::cluster_edge op{};

  // act
  op(population_, clusters_);

  // assert
  for (std::size_t i = 0; i < 8; ++i) {
    EXPECT_EQ(get_ranking(population_, i), i / 4 + 1);
    EXPECT_EQ(get_cluster(population_, i), i);
  }
}

TEST_F(cluster_edge_tests, pruning_set_content) {
  // arrange
  gal::prune::cluster_edge op{};

  // act
  op(population_, clusters_);

  // assert
  std::vector all(clusters_.begin(), clusters_.end());
  std::vector<gal::cluster_set::cluster> expected{
      {1, 1}, {1, 1}, {1, 1}, {1, 1}, {2, 1}, {2, 1}, {2, 1}, {2, 1}};

  EXPECT_THAT(all, ::testing::ElementsAreArray(expected));
}

} // namespace tests::pruning
