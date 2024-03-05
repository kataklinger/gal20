
#include <scaling.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::scaling {

using fitness_cmp_t = gal::maximize<gal::max_floatingpoint_three_way>;

struct no_tags {};

using population_t =
    gal::population<int, double, fitness_cmp_t, double, fitness_cmp_t, no_tags>;

using statistics_t = gal::stats::statistics<population_t,
                                            gal::stats::fitness_deviation_raw,
                                            gal::stats::average_fitness_raw,
                                            gal::stats::total_fitness_raw,
                                            gal::stats::extreme_fitness_raw>;

using history_t = gal::stats::history<statistics_t>;

using individual_t = population_t::individual_t;
using evaluation_t = individual_t::evaluation_t;

template<typename Individuals, typename Fn>
auto transform_result(Individuals const& individuals, Fn&& fn) {
  auto transformed = individuals | std::views::transform(std::forward<Fn>(fn));

  using item_t = std::ranges::range_value_t<decltype(transformed)>;

  return std::vector<item_t>(std::ranges::begin(transformed),
                             std::ranges::end(transformed));
}

template<typename Population>
auto get_scaled_fitness(Population const& population) {
  return transform_result(population.individuals(),
                          [](auto& i) { return i.eval().scaled(); });
}

template<typename Population, typename Scaling>
void invoke_scaling(Population& population, Scaling&& scaling) {
  if constexpr (std::is_invocable_v<Scaling>) {
    std::invoke(scaling);
  }

  for (std::size_t ordinal{}; auto&& individual : population.individuals()) {
    std::invoke(scaling, ordinal++, individual);
  }
}

struct scaling_tests : public ::testing::Test {
protected:
  void SetUp() override {
    std::vector<individual_t> individuals{{0, evaluation_t{1., 0.}, no_tags{}},
                                          {1, evaluation_t{2., 0.}, no_tags{}},
                                          {2, evaluation_t{3., 0.}, no_tags{}},
                                          {3, evaluation_t{4., 0.}, no_tags{}},
                                          {4, evaluation_t{5., 0.}, no_tags{}}};

    population_.insert(individuals);
    history_.next(population_);
  }

  population_t population_{{}, {}, true};
  history_t history_{1};

  gal::population_context<population_t, statistics_t> context_{population_,
                                                               history_};
};

TEST_F(scaling_tests, linear) {
  using namespace gal::literals;

  // arrange
  auto op = gal::scale::factory<gal::scale::linear, 4._fc>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(0, 1.5, 3, 4.5, 6));
}

TEST_F(scaling_tests, sigma) {
  // arrange
  auto op = gal::scale::factory<gal::scale::sigma>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(0.29289321881345254,
                                     0.64644660940672627,
                                     1,
                                     1.3535533905932737,
                                     1.7071067811865475));
}

TEST_F(scaling_tests, ranked) {
  using namespace gal::literals;

  // arrange
  auto op = gal::scale::factory<gal::scale::ranked, 2._fc>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(2., 1.5, 1., 0.5, 0.));
}

TEST_F(scaling_tests, exponential) {
  using namespace gal::literals;

  // arrange
  auto op = gal::scale::factory<gal::scale::exponential, 2._fc>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(16., 8., 4., 2., 1.));
}

TEST_F(scaling_tests, top) {
  using namespace gal::literals;

  // arrange
  auto op = gal::scale::factory<gal::scale::top, 3, 2._fc>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(10., 8., 6., 4., 0.));
}

TEST_F(scaling_tests, power) {
  using namespace gal::literals;

  // arrange
  auto op = gal::scale::factory<gal::scale::power, 3._fc>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(1., 8., 27., 64., 125.));
}

TEST_F(scaling_tests, window) {
  // arrange
  auto op = gal::scale::factory<gal::scale::window>{}(context_);

  // act
  invoke_scaling(population_, op);

  // assert
  EXPECT_THAT(get_scaled_fitness(population_),
              ::testing::ElementsAre(0., 1., 2., 3., 4.));
}

} // namespace tests::scaling
