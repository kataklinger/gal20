
#include "crowding.hpp"

#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::crowding {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t = std::tuple<gal::frontier_level_t,
                          gal::int_rank_t,
                          gal::crowd_density_t,
                          gal::cluster_label>;

using population_t = gal::population<fitness_t,
                                     fitness_t,
                                     cmp_t,
                                     double,
                                     gal::disabled_comparator,
                                     tags_t>;

template<typename Population>
inline auto get_all_density(Population const& population) {
  auto transformed =
      population.individuals() | std::views::transform([](auto const& i) {
        return gal::get_tag<gal::crowd_density_t>(i).get();
      });

  return std::vector<double>(std::ranges::begin(transformed),
                             std::ranges::end(transformed));
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

class crowding_base_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr tags_t tags{0, 0, 0, 0};

    std::vector<individual_t> individuals{{f1a, evaluation_t{f1a}, tags},
                                          {f1b, evaluation_t{f1b}, tags},
                                          {f1c, evaluation_t{f1c}, tags},
                                          {f1d, evaluation_t{f1d}, tags},
                                          {f1e, evaluation_t{f1e}, tags},
                                          {f1f, evaluation_t{f1f}, tags},
                                          {f1g, evaluation_t{f1g}, tags},
                                          {f1h, evaluation_t{f1h}, tags},
                                          {f2a, evaluation_t{f2a}, tags},
                                          {f2b, evaluation_t{f2b}, tags},
                                          {f2c, evaluation_t{f2c}, tags},
                                          {f2d, evaluation_t{f2d}, tags},
                                          {f2e, evaluation_t{f2e}, tags},
                                          {f2f, evaluation_t{f2f}, tags},
                                          {f2g, evaluation_t{f2g}, tags},
                                          {f2h, evaluation_t{f2h}, tags}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};
};

class sharing_crowding_tests : public crowding_base_tests {
protected:
  gal::cluster_set clusters_{};
};

TEST_F(sharing_crowding_tests, population_labels) {
  // arrange
  gal::crowd::sharing op{
      0.3, 1., [](fitness_t const& lhs, fitness_t const& rhs) {
        return gal::euclidean_distance(lhs, rhs);
      }};

  auto pareto = gal::rank::level{}(population_, gal::pareto_preserved_tag);

  // act
  op(population_, pareto, clusters_);

  // assert
  auto result = get_all_density(population_);
  EXPECT_THAT(result,
              ::testing::ElementsAre(0.26283019707021149,
                                     0.47433960585957713,
                                     0.26283019707021127,
                                     0,
                                     0,
                                     0.26283019707021127,
                                     0.47433960585957713,
                                     0.26283019707021149,
                                     0.13141509853510575,
                                     0.23716980292978859,
                                     0.13141509853510586,
                                     0,
                                     0,
                                     0.13141509853510586,
                                     0.23716980292978859,
                                     0.13141509853510575));
}

class distance_crowding_tests : public crowding_base_tests {
protected:
  gal::cluster_set clusters_{};
};

TEST_F(distance_crowding_tests, population_labels) {
  // arrange
  gal::crowd::distance op{};
  auto pareto = gal::rank::level{}(population_, gal::pareto_preserved_tag);

  // act
  op(population_, pareto, clusters_);

  // assert
  auto result = get_all_density(population_);
  EXPECT_THAT(result,
              ::testing::ElementsAre(0,
                                     0.28571428571428564,
                                     0.18181818181818177,
                                     0.14285714285714282,
                                     0.14285714285714282,
                                     0.18181818181818177,
                                     0.28571428571428564,
                                     0,
                                     0,
                                     0.28571428571428553,
                                     0.18181818181818171,
                                     0.14285714285714274,
                                     0.14285714285714274,
                                     0.18181818181818171,
                                     0.28571428571428553,
                                     0));
}

class neighbor_crowding_tests : public crowding_base_tests {
protected:
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
  gal::cluster_set clusters_{};
};

TEST_F(neighbor_crowding_tests, population_labels) {
  // arrange
  gal::crowd::neighbor op{};

  // act
  op(population_, pareto_, clusters_);

  // assert
  auto result = get_all_density(population_);
  EXPECT_THAT(result,
              ::testing::ElementsAre(0.29051015094500221,
                                     0.29228937355798507,
                                     0.29228937355798507,
                                     0.29289321881345248,
                                     0.30554949322025815,
                                     0.29289321881345248,
                                     0.29289321881345248,
                                     0.29228937355798507,
                                     0.29051015094500221,
                                     0.29289321881345248,
                                     0.29289321881345248,
                                     0.30554949322025815,
                                     0.30554949322025815,
                                     0.29289321881345248,
                                     0.29289321881345248,
                                     0.29228937355798507));
}

class cluster_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {fit, evaluation_t{fit}, tags_t{1, 1, 0, 0}},
        {fit, evaluation_t{fit}, tags_t{1, 1, 0, 0}},
        {fit, evaluation_t{fit}, tags_t{1, 1, 0, 1}},
        {fit, evaluation_t{fit}, tags_t{1, 1, 0, 1}},
        {fit, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {fit, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {fit, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {fit, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {fit, evaluation_t{fit}, tags_t{3, 3, 0, 3}}};

    clusters_.next_level();
    clusters_.add_cluster(2);
    clusters_.add_cluster(2);

    clusters_.next_level();
    clusters_.add_cluster(4);

    clusters_.next_level();
    clusters_.add_cluster(1);

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
  gal::cluster_set clusters_{};
};

TEST_F(cluster_tests, population_density_lables) {
  // arrange
  gal::crowd::cluster<1.> op{};

  // act
  op(population_, pareto_, clusters_);

  // assert
  auto result = get_all_density(population_);
  EXPECT_THAT(
      result,
      ::testing::ElementsAre(0.5, 0.5, 0.5, 0.5, 0.75, 0.75, 0.75, 0.75, 0.));
}

} // namespace tests::crowding
