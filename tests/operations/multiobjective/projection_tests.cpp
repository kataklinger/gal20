
#include "projection.hpp"

#include "context.hpp"
#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::projection {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t =
    std::tuple<gal::frontier_level_t, gal::int_rank_t, gal::crowd_density_t>;

constexpr fitness_t f1a{0, 1};
constexpr fitness_t f1b{0, 1};
constexpr fitness_t f2a{2, 1};
constexpr fitness_t f2b{1, 2};
constexpr fitness_t f3a{3, 2};
constexpr fitness_t f3b{2, 3};

template<std::ranges::range R>
constexpr auto to_vector(R&& r) {
  auto extracted =
      std::forward<R>(r) |
      std::views::transform([](auto const& i) { return i.eval().scaled(); });

  using elem_t = std::decay_t<std::ranges::range_value_t<decltype(extracted)>>;
  return std::vector<elem_t>{std::ranges::begin(extracted),
                             std::ranges::end(extracted)};
}

class projection_tests : public ::testing::Test {
protected:
  using population_t = gal::population<int,
                                       fitness_t,
                                       cmp_t,
                                       double,
                                       gal::disabled_comparator,
                                       tags_t>;

  using statistics_t =
      gal::stats::statistics<population_t, gal::stats::generation>;
  using population_ctx_t = gal::population_context<population_t, statistics_t>;

  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{1, 1, 0.75}},
        {0, evaluation_t{f1b}, tags_t{1, 1, 0.5}},
        {0, evaluation_t{f2a}, tags_t{2, 2, 0.25}},
        {0, evaluation_t{f2b}, tags_t{2, 2, 0.125}},
        {0, evaluation_t{f3a}, tags_t{3, 3, 0.05}},
        {0, evaluation_t{f3b}, tags_t{3, 3, 0.025}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};
  gal::stats::history<statistics_t> statistics_{1};
  population_ctx_t context_{population_, statistics_};
};

TEST_F(projection_tests, scale_scaled_fitness_value) {
  // arrange
  gal::project::scale<gal::int_rank_t> op{};
  auto pareto = gal::rank::level{}(population_, gal::pareto_preserved_tag);

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1.4035, 0.0001);
  EXPECT_NEAR(values[1], 2.8070, 0.0001);
  EXPECT_NEAR(values[2], 1.0526, 0.0001);
  EXPECT_NEAR(values[3], 1.2280, 0.0001);
  EXPECT_NEAR(values[4], 0.9999, 0.0001);
  EXPECT_NEAR(values[5], 1.0263, 0.0001);
}
TEST_F(projection_tests, translate_scaled_fitness_value) {
  // arrange
  gal::project::translate<gal::int_rank_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1.25, 0.000001);
  EXPECT_NEAR(values[1], 1.5, 0.000001);
  EXPECT_NEAR(values[2], 2.75, 0.000001);
  EXPECT_NEAR(values[3], 2.875, 0.000001);
  EXPECT_NEAR(values[4], 3.95, 0.000001);
  EXPECT_NEAR(values[5], 3.975, 0.000001);
}

TEST_F(projection_tests, truncate_ranking_scaled_fitness_value) {
  // arrange
  gal::project::truncate<gal::int_rank_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1, 0.000001);
  EXPECT_NEAR(values[1], 1, 0.000001);
  EXPECT_NEAR(values[2], 2, 0.000001);
  EXPECT_NEAR(values[3], 2, 0.000001);
  EXPECT_NEAR(values[4], 3, 0.000001);
  EXPECT_NEAR(values[5], 3, 0.000001);
}

TEST_F(projection_tests, truncate_density_scaled_fitness_value) {
  // arrange
  gal::project::truncate<gal::crowd_density_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 0.25, 0.0001);
  EXPECT_NEAR(values[1], 0.5, 0.0001);
  EXPECT_NEAR(values[2], 0.75, 0.0001);
  EXPECT_NEAR(values[3], 0.875, 0.0001);
  EXPECT_NEAR(values[4], 0.95, 0.0001);
  EXPECT_NEAR(values[5], 0.975, 0.0001);
}

TEST_F(projection_tests, alternate_ranking_scaled_fitness_value) {
  // arrange
  gal::project::alternate<gal::int_rank_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1, 0.000001);
  EXPECT_NEAR(values[1], 1, 0.000001);
  EXPECT_NEAR(values[2], 2, 0.000001);
  EXPECT_NEAR(values[3], 2, 0.000001);
  EXPECT_NEAR(values[4], 3, 0.000001);
  EXPECT_NEAR(values[5], 3, 0.000001);
}

TEST_F(projection_tests, alternate_density_scaled_fitness_value) {
  // arrange
  gal::project::alternate<gal::crowd_density_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};
  statistics_.next(population_);

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 0.25, 0.0001);
  EXPECT_NEAR(values[1], 0.5, 0.0001);
  EXPECT_NEAR(values[2], 0.75, 0.0001);
  EXPECT_NEAR(values[3], 0.875, 0.0001);
  EXPECT_NEAR(values[4], 0.95, 0.0001);
  EXPECT_NEAR(values[5], 0.975, 0.0001);
}

TEST_F(projection_tests, custom_scaled_fitness_value) {
  // arrange
  gal::project::custom op{[](auto const&) { return 1.1; }};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1.1, 0.0001);
  EXPECT_NEAR(values[1], 1.1, 0.0001);
  EXPECT_NEAR(values[2], 1.1, 0.0001);
  EXPECT_NEAR(values[3], 1.1, 0.0001);
  EXPECT_NEAR(values[4], 1.1, 0.0001);
  EXPECT_NEAR(values[5], 1.1, 0.0001);
}

class merge_tests : public ::testing::Test {
protected:
  using population_t = gal::population<int,
                                       fitness_t,
                                       cmp_t,
                                       std::tuple<std::size_t, double>,
                                       gal::disabled_comparator,
                                       tags_t>;

  using statistics_t =
      gal::stats::statistics<population_t, gal::stats::generation>;
  using population_ctx_t = gal::population_context<population_t, statistics_t>;

  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{1, 1, 0.75}},
        {0, evaluation_t{f1b}, tags_t{1, 1, 0.5}},
        {0, evaluation_t{f2a}, tags_t{2, 2, 0.25}},
        {0, evaluation_t{f2b}, tags_t{2, 2, 0.125}},
        {0, evaluation_t{f3a}, tags_t{3, 3, 0.05}},
        {0, evaluation_t{f3b}, tags_t{3, 3, 0.025}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};
  gal::stats::history<statistics_t> statistics_{1};
  population_ctx_t context_{population_, statistics_};
};

TEST_F(merge_tests, merge_scaled_fitness_value) {
  // arrange
  gal::project::merge<gal::int_rank_t> op{};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(context_, pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_EQ(std::get<0>(values[0]), 1);
  EXPECT_EQ(std::get<0>(values[1]), 1);
  EXPECT_EQ(std::get<0>(values[2]), 2);
  EXPECT_EQ(std::get<0>(values[3]), 2);
  EXPECT_EQ(std::get<0>(values[4]), 3);
  EXPECT_EQ(std::get<0>(values[5]), 3);

  EXPECT_NEAR(std::get<1>(values[0]), 0.25, 0.000001);
  EXPECT_NEAR(std::get<1>(values[1]), 0.5, 0.000001);
  EXPECT_NEAR(std::get<1>(values[2]), 0.75, 0.000001);
  EXPECT_NEAR(std::get<1>(values[3]), 0.875, 0.000001);
  EXPECT_NEAR(std::get<1>(values[4]), 0.95, 0.000001);
  EXPECT_NEAR(std::get<1>(values[5]), 0.975, 0.000001);
}

} // namespace tests::projection
