

#include "pruning.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::pruning {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t = std::tuple<gal::frontier_level_t,
                          gal::int_rank_t,
                          gal::crowd_density_t,
                          gal::cluster_label>;

using population_t = gal::
    population<int, fitness_t, cmp_t, double, gal::disabled_comparator, tags_t>;

template<typename Tag, typename Population>
inline auto get_tag_value(Population const& population, std::size_t index) {
  return gal::get_tag<Tag>(population.individuals()[index]).get();
}

template<typename Population>
inline auto get_crowd_density(Population const& population, std::size_t index) {
  return get_tag_value<gal::crowd_density_t>(population, index);
}

template<typename Population>
inline auto get_ranking(Population const& population, std::size_t index) {
  return get_tag_value<gal::int_rank_t>(population, index);
}

class global_worst_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {0, evaluation_t{fit}, tags_t{3, 3, 0.1, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.4, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.3, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.2, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.1, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.3, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.2, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.1, 0}}};

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
        {0, evaluation_t{fit}, tags_t{3, 3, 0.1, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.4, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.3, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.2, 0}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0.1, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.3, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.2, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0.1, 0}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, 5, false};
};

} // namespace tests::pruning
