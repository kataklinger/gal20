
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

class neighbor_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{f1b}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{f1c}, tags_t{1, 1, 0, 0}},

        {0, evaluation_t{f1d}, tags_t{1, 1, 0, 0}},

        {0, evaluation_t{f1e}, tags_t{1, 1, 0, 0}},

        {0, evaluation_t{f1f}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{f1g}, tags_t{1, 1, 0, 0}},
        {0, evaluation_t{f1h}, tags_t{1, 1, 0, 0}},

        {0, evaluation_t{f2a}, tags_t{2, 2, 0, 0}},
        {0, evaluation_t{f2b}, tags_t{2, 2, 0, 0}},
        {0, evaluation_t{f2c}, tags_t{2, 2, 0, 0}},

        {0, evaluation_t{f2d}, tags_t{2, 2, 0, 0}},

        {0, evaluation_t{f2e}, tags_t{2, 2, 0, 0}},

        {0, evaluation_t{f2f}, tags_t{2, 2, 0, 0}},
        {0, evaluation_t{f2g}, tags_t{2, 2, 0, 0}},
        {0, evaluation_t{f2h}, tags_t{2, 2, 0, 0}}};

    population_.insert(individuals);
  }

  population_t population_{cmp_t{}, gal::disabled_comparator{}, false};
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
  gal::cluster_set clusters_{};
};

TEST_F(neighbor_tests, population_labels) {
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
