
#include "projection.hpp"

#include "context.hpp"
#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::projection {

using raw_cmp_t = gal::dominate<std::less<>>;
using scaled_cmp_t = gal::floatingpoint_three_way;

using raw_fitness_t = std::array<double, 2>;
using scaled_fitness_t = double;

using tags_t =
    std::tuple<gal::frontier_level_t, gal::int_rank_t, gal::crowd_density_t>;

using population_t = gal::population<int,
                                     raw_fitness_t,
                                     raw_cmp_t,
                                     scaled_fitness_t,
                                     scaled_cmp_t,
                                     tags_t>;

using statistics_t = gal::stats::statistics<population_t, gal::stats::blank>;
using population_ctx_t = gal::population_context<population_t, statistics_t>;

constexpr raw_fitness_t fa1{0, 1};
constexpr raw_fitness_t f1b{0, 1};
constexpr raw_fitness_t f2a{2, 1};
constexpr raw_fitness_t f2b{1, 2};
constexpr raw_fitness_t f3a{3, 2};
constexpr raw_fitness_t f3b{2, 3};

template<std::ranges::range R>
constexpr auto to_vector(R&& r) {
  auto extracted =
      std::forward<R>(r) | std::views::transform([](auto const& i) {
        return i.evaluation().scaled();
      });

  using elem_t = std::decay_t<std::ranges::range_value_t<decltype(extracted)>>;
  return std::vector<elem_t>{std::ranges::begin(extracted),
                             std::ranges::end(extracted)};
}

class projection_tests : public testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;
    raw_fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {0, evaluation_t{fa1}, tags_t{1, 1, 0.75}},
        {0, evaluation_t{f1b}, tags_t{1, 1, 0.5}},
        {0, evaluation_t{f2a}, tags_t{2, 2, 0.25}},
        {0, evaluation_t{f2b}, tags_t{2, 2, 0.125}},
        {0, evaluation_t{f3a}, tags_t{3, 3, 0.05}},
        {0, evaluation_t{f3b}, tags_t{3, 3, 0.025}}};

    population_.insert(individuals);
  }

  population_t population_{raw_cmp_t{}, scaled_cmp_t{}, false};
  gal::stats::history<statistics_t> statistics_{1};
  population_ctx_t context_{population_, statistics_};
};

TEST_F(projection_tests, scale_scaled_fitness_value) {
  // arrange
  gal::project::scale<population_ctx_t, gal::int_rank_t> op{context_};
  auto pareto = gal::rank::level{}(population_, gal::pareto_preserved_tag);

  // act
  op(pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 120., 0.0000001);
  EXPECT_NEAR(values[1], 80., 0.0000001);
  EXPECT_NEAR(values[2], 2.5, 0.0000001);
  EXPECT_NEAR(values[3], 1.25, 0.0000001);
  EXPECT_NEAR(values[4], 0.00375, 0.0000001);
  EXPECT_NEAR(values[5], 0.001875, 0.0000001);
}

TEST_F(projection_tests, translate_scaled_fitness_value) {
  // arrange
  gal::project::translate<population_ctx_t, gal::int_rank_t> op{context_};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto{0};

  // act
  op(pareto, gal::cluster_set{});

  // assert
  auto values = to_vector(population_.individuals());
  EXPECT_NEAR(values[0], 1.75, 0.000001);
  EXPECT_NEAR(values[1], 1.5, 0.000001);
  EXPECT_NEAR(values[2], 2.25, 0.000001);
  EXPECT_NEAR(values[3], 2.125, 0.000001);
  EXPECT_NEAR(values[4], 3.049999, 0.000001);
  EXPECT_NEAR(values[5], 3.024999, 0.000001);
}

} // namespace tests::projection
