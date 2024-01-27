
#include "crowding.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::crowding {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t = std::tuple<gal::frontier_level_t,
                          gal::int_rank_t,
                          gal::crowd_density_t,
                          gal::cluster_label>;

using population_t = gal::
    population<int, fitness_t, cmp_t, double, gal::disabled_comparator, tags_t>;

template<typename Population>
inline auto get_all_density(Population const& population) {
  auto transformed =
      population.individuals() | std::views::transform([](auto const& i) {
        return gal::get_tag<gal::crowd_density_t>(i).get();
      });

  return std::vector<double>(std::ranges::begin(transformed),
                             std::ranges::end(transformed));
}

class cluster_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    constexpr fitness_t fit{0, 0};

    std::vector<individual_t> individuals{
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 1}},
        {0, evaluation_t{fit}, tags_t{1, 1, 0, 1}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {0, evaluation_t{fit}, tags_t{2, 2, 0, 2}},
        {0, evaluation_t{fit}, tags_t{3, 3, 0, 3}}};

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
