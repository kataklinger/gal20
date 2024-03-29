
#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::ranking {

using raw_cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

template<typename Ranking>
using population_t =
    gal::population<int,
                    fitness_t,
                    raw_cmp_t,
                    gal::empty_fitness,
                    gal::disabled_comparator,
                    std::tuple<gal::frontier_level_t, Ranking>>;

constexpr fitness_t f1a{0, 0};
constexpr fitness_t f2a{1, 0};
constexpr fitness_t f2b{0, 1};
constexpr fitness_t f3a{1, 1};

template<typename Tag, typename Population>
inline auto get_tag_value(Population const& population, std::size_t index) {
  return gal::get_tag<Tag>(population.individuals()[index]).get();
}

template<typename Population>
inline auto get_frontier_level(Population const& population,
                               std::size_t index) {
  return get_tag_value<gal::frontier_level_t>(population, index);
}

template<typename Ranking, typename Population>
inline auto get_ranking(Population const& population, std::size_t index) {
  return get_tag_value<Ranking>(population, index);
}

template<std::ranges::range R>
constexpr auto to_vector(R&& r) {
  auto extracted =
      std::forward<R>(r) |
      std::views::transform([](auto const& i) { return i->eval().raw(); });

  using elem_t = std::decay_t<std::ranges::range_value_t<decltype(extracted)>>;
  return std::vector<elem_t>{std::ranges::begin(extracted),
                             std::ranges::end(extracted)};
}

class binary_ranking_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t<gal::bin_rank_t>::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{f1a}},
                                          {0, evaluation_t{f2a}},
                                          {0, evaluation_t{f2b}},
                                          {0, evaluation_t{f3a}}};

    population_4_.insert(individuals);
  }

  population_t<gal::bin_rank_t> population_4_{raw_cmp_t{},
                                              gal::disabled_comparator{},
                                              false};

  population_t<gal::bin_rank_t> population_0_{raw_cmp_t{},
                                              gal::disabled_comparator{},
                                              false};
};

TEST_F(binary_ranking_tests, preserved_empty_population) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_0_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(binary_ranking_tests, preserved_front_sizes) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(binary_ranking_tests, preserved_front_content) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(binary_ranking_tests, preserved_ranking_tags) {
  // arrange
  gal::rank::binary op{};

  // act
  op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 0),
            gal::binary_rank::nondominated);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 1),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 2),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 3),
            gal::binary_rank::dominated);
}

TEST_F(binary_ranking_tests, reduced_empty_population) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_0_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(binary_ranking_tests, reduced_front_sizes) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(binary_ranking_tests, reduced_front_content) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(binary_ranking_tests, reduced_ranking_tags) {
  // arrange
  gal::rank::binary op{};

  // act
  op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 0),
            gal::binary_rank::nondominated);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 1),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 2),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 3),
            gal::binary_rank::dominated);
}

TEST_F(binary_ranking_tests, nondominated_empty_population) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_0_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(binary_ranking_tests, nondominated_front_sizes) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results.get_size_of(1), 1);
}

TEST_F(binary_ranking_tests, nondominated_front_content) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));
}

TEST_F(binary_ranking_tests, nondominated_ranking_tags) {
  // arrange
  gal::rank::binary op{};

  // act
  op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 0),
            gal::binary_rank::nondominated);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 1),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 2),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 3),
            gal::binary_rank::dominated);
}

TEST_F(binary_ranking_tests, erased_empty_population) {
  // arrange
  gal::rank::binary op{};

  // act
  auto results = op(population_0_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(binary_ranking_tests, erased_ranking_tags) {
  // arrange
  gal::rank::binary op{};

  // act
  op(population_4_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 0),
            gal::binary_rank::nondominated);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 1),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 2),
            gal::binary_rank::dominated);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::bin_rank_t>(population_4_, 3),
            gal::binary_rank::dominated);
}

class level_ranking_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t<gal::int_rank_t>::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{f1a}},
                                          {0, evaluation_t{f2a}},
                                          {0, evaluation_t{f2b}},
                                          {0, evaluation_t{f3a}}};

    population_4_.insert(individuals);
  }

  population_t<gal::int_rank_t> population_4_{raw_cmp_t{},
                                              gal::disabled_comparator{},
                                              false};

  population_t<gal::int_rank_t> population_0_{raw_cmp_t{},
                                              gal::disabled_comparator{},
                                              false};
};

TEST_F(level_ranking_tests, preserved_empty_population) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_0_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(level_ranking_tests, preserved_front_sizes) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 2);
  EXPECT_EQ(results.get_size_of(3), 1);
}

TEST_F(level_ranking_tests, preserved_front_content) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f3a));
}

TEST_F(level_ranking_tests, preserved_ranking_tags) {
  // arrange
  gal::rank::level op{};

  // act
  op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 3);
}

TEST_F(level_ranking_tests, reduced_empty_population) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_0_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(level_ranking_tests, reduced_front_sizes) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(level_ranking_tests, reduced_front_content) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(level_ranking_tests, reduced_ranking_tags) {
  // arrange
  gal::rank::level op{};

  // act
  op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 3);
}

TEST_F(level_ranking_tests, nondominated_empty_population) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_0_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(level_ranking_tests, nondominated_front_sizes) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results.get_size_of(1), 1);
}

TEST_F(level_ranking_tests, nondominated_front_content) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));
}

TEST_F(level_ranking_tests, nondominated_ranking_tags) {
  // arrange
  gal::rank::level op{};

  // act
  op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 3);
}

TEST_F(level_ranking_tests, erased_empty_population) {
  // arrange
  gal::rank::level op{};

  // act
  auto results = op(population_0_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(level_ranking_tests, erased_ranking_tags) {
  // arrange
  gal::rank::level op{};

  // act
  op(population_4_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 3);
}

class acc_level_ranking_tests : public level_ranking_tests {};

TEST_F(acc_level_ranking_tests, preserved_empty_population) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_0_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_level_ranking_tests, preserved_front_sizes) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 2);
  EXPECT_EQ(results.get_size_of(3), 1);
}

TEST_F(acc_level_ranking_tests, preserved_front_content) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f3a));
}

TEST_F(acc_level_ranking_tests, preserved_ranking_tags) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 6);
}

TEST_F(acc_level_ranking_tests, reduced_empty_population) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_0_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_level_ranking_tests, reduced_front_sizes) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(acc_level_ranking_tests, reduced_front_content) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(acc_level_ranking_tests, reduced_ranking_tags) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 6);
}

TEST_F(acc_level_ranking_tests, nondominated_empty_population) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_0_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_level_ranking_tests, nondominated_front_sizes) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results.get_size_of(1), 1);
}

TEST_F(acc_level_ranking_tests, nondominated_front_content) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));
}

TEST_F(acc_level_ranking_tests, nondominated_ranking_tags) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 6);
}

TEST_F(acc_level_ranking_tests, erased_empty_population) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  auto results = op(population_0_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_level_ranking_tests, erased_ranking_tags) {
  // arrange
  gal::rank::accumulated_level op{};

  // act
  op(population_4_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 1);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 2);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 6);
}

class strength_ranking_tests : public ::testing::Test {
protected:
  void SetUp() override {
    using individual_t = population_t<gal::real_rank_t>::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{{0, evaluation_t{f1a}},
                                          {0, evaluation_t{f2a}},
                                          {0, evaluation_t{f2b}},
                                          {0, evaluation_t{f3a}}};

    population_4_.insert(individuals);
  }

  population_t<gal::real_rank_t> population_4_{raw_cmp_t{},
                                               gal::disabled_comparator{},
                                               false};

  population_t<gal::real_rank_t> population_0_{raw_cmp_t{},
                                               gal::disabled_comparator{},
                                               false};
};

TEST_F(strength_ranking_tests, preserved_empty_population) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_0_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(strength_ranking_tests, preserved_front_sizes) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 2);
  EXPECT_EQ(results.get_size_of(3), 1);
}

TEST_F(strength_ranking_tests, preserved_front_content) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f3a));
}

TEST_F(strength_ranking_tests, preserved_ranking_tags) {
  // arrange
  gal::rank::strength op{};

  // act
  op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 0), 0.75);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 1), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 2), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 3), 1.75);
}

TEST_F(strength_ranking_tests, reduced_empty_population) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_0_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(strength_ranking_tests, reduced_front_sizes) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(strength_ranking_tests, reduced_front_content) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(strength_ranking_tests, reduced_ranking_tags) {
  // arrange
  gal::rank::strength op{};

  // act
  op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 0), 0.75);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 1), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 2), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 3), 1.75);
}

TEST_F(strength_ranking_tests, nondominated_empty_population) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_0_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(strength_ranking_tests, nondominated_front_sizes) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results.get_size_of(1), 1);
}

TEST_F(strength_ranking_tests, nondominated_front_content) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));
}

TEST_F(strength_ranking_tests, nondominated_ranking_tags) {
  // arrange
  gal::rank::strength op{};

  // act
  op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 0), 0.75);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 1), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 2), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 3), 1.75);
}

TEST_F(strength_ranking_tests, erased_empty_population) {
  // arrange
  gal::rank::strength op{};

  // act
  auto results = op(population_0_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(strength_ranking_tests, erased_ranking_tags) {
  // arrange
  gal::rank::strength op{};

  // act
  op(population_4_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 0), 0.75);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 1), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 2), 1.75);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::real_rank_t>(population_4_, 3), 1.75);
}

class acc_strength_ranking_tests : public level_ranking_tests {};

TEST_F(acc_strength_ranking_tests, preserved_empty_population) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_0_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_strength_ranking_tests, preserved_front_sizes) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(results.size(), 3);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 2);
  EXPECT_EQ(results.get_size_of(3), 1);
}

TEST_F(acc_strength_ranking_tests, preserved_front_content) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_preserved_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f3a));
}

TEST_F(acc_strength_ranking_tests, preserved_ranking_tags) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  op(population_4_, gal::pareto_preserved_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 0);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 3);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 5);
}

TEST_F(acc_strength_ranking_tests, reduced_empty_population) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_0_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_strength_ranking_tests, reduced_front_sizes) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results.get_size_of(1), 1);
  EXPECT_EQ(results.get_size_of(2), 3);
}

TEST_F(acc_strength_ranking_tests, reduced_front_content) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_reduced_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(*it), ::testing::UnorderedElementsAre(f2a, f2b, f3a));
}

TEST_F(acc_strength_ranking_tests, reduced_ranking_tags) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  op(population_4_, gal::pareto_reduced_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 0);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 5);
}

TEST_F(acc_strength_ranking_tests, nondominated_empty_population) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_0_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_strength_ranking_tests, nondominated_front_sizes) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results.get_size_of(1), 1);
}

TEST_F(acc_strength_ranking_tests, nondominated_front_content) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_4_, gal::pareto_nondominated_tag);

  // assert
  auto it = results.begin();
  EXPECT_THAT(to_vector(*it), ::testing::ElementsAre(f1a));
}

TEST_F(acc_strength_ranking_tests, nondominated_ranking_tags) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  op(population_4_, gal::pareto_nondominated_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 0);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 5);
}

TEST_F(acc_strength_ranking_tests, erased_empty_population) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  auto results = op(population_0_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(results.size(), 0);
}

TEST_F(acc_strength_ranking_tests, erased_ranking_tags) {
  // arrange
  gal::rank::accumulated_strength op{};

  // act
  op(population_4_, gal::pareto_erased_tag);

  // assert
  EXPECT_EQ(get_frontier_level(population_4_, 0), 1);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 0), 0);

  EXPECT_EQ(get_frontier_level(population_4_, 1), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 1), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 2), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 2), 3);

  EXPECT_EQ(get_frontier_level(population_4_, 3), 2);
  EXPECT_EQ(get_ranking<gal::int_rank_t>(population_4_, 3), 5);
}

} // namespace tests::ranking
