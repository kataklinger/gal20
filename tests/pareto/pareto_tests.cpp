
#include "fitness.hpp"
#include "pareto.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

using individual_t = std::array<int, 2>;

static gal::dominate cmp{std::less{}};

template<std::ranges::range R>
constexpr auto to_vector(R&& r) {
  auto extracted =
      std::forward<R>(r) |
      std::views::transform([](auto const& s) { return s.individual(); });

  using elem_t = std::decay_t<std::ranges::range_value_t<decltype(extracted)>>;
  return std::vector<elem_t>{std::ranges::begin(extracted),
                             std::ranges::end(extracted)};
}

class pareto_sort_tests : public testing::Test {
protected:
  inline static constexpr individual_t f1a{0, 0};
  inline static constexpr individual_t f2a{1, 0};
  inline static constexpr individual_t f2b{0, 1};
  inline static constexpr individual_t f3a{1, 1};

  void SetUp() override {
  }

  std::vector<individual_t> individuals_0_{0};
  std::vector<individual_t> individuals_1_{f1a};
  std::vector<individual_t> individuals_5_{f1a, f2a, f2b, f3a};
};

TEST_F(pareto_sort_tests, pareto_multi_front_count) {
  // arrange
  gal::dominate cmp{std::less{}};

  // act
  auto sorted = individuals_5_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 3);
}

TEST_F(pareto_sort_tests, pareto_multi_front_ordering) {
  // arrange

  // act
  auto sorted = individuals_5_ | gal::pareto::views::sort(cmp);

  // assert
  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(it->members()),
              ::testing::UnorderedElementsAre(f2a, f2b));
  ++it;
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f3a));
}

TEST_F(pareto_sort_tests, pareto_single_front_count) {
  // arrange

  // act
  auto sorted = individuals_1_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 1);
}

TEST_F(pareto_sort_tests, pareto_single_front_ordering) {
  // arrange

  // act
  auto sorted = individuals_1_ | gal::pareto::views::sort(cmp);

  // assert
  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f1a));
}

TEST_F(pareto_sort_tests, pareto_empty_front_count) {
  // arrange

  // act
  auto sorted = individuals_0_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 1);

  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::IsEmpty());
}

TEST_F(pareto_sort_tests, pareto_multi_analyze) {
  // arrange

  // act
  auto nondominated = gal::pareto::analyze(individuals_5_, cmp);

  //   assert
  auto it = std::ranges::begin(nondominated);
  auto indv = *it;
  EXPECT_TRUE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()),
              ::testing::UnorderedElementsAre(f2a, f2b, f3a));

  indv = *++it;
  EXPECT_FALSE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()),
              ::testing::UnorderedElementsAre(f3a));

  indv = *++it;
  EXPECT_FALSE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()),
              ::testing::UnorderedElementsAre(f3a));

  indv = *++it;
  EXPECT_FALSE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()), ::testing::IsEmpty());
}

TEST_F(pareto_sort_tests, pareto_single_analyze) {
  // arrange

  // act
  auto nondominated = gal::pareto::analyze(individuals_1_, cmp);

  //   assert
  auto it = std::ranges::begin(nondominated);
  auto indv = *it;
  EXPECT_TRUE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()), ::testing::IsEmpty());
}

TEST_F(pareto_sort_tests, pareto_empty_analyze) {
  // arrange

  // act
  auto nondominated = gal::pareto::analyze(individuals_0_, cmp);

  //   assert
  EXPECT_EQ(std::ranges::distance(nondominated), 0);
}

struct tracker {
  tracker(std::vector<individual_t> const& individuals)
      : first_{&individuals.front()}
      , flags_(individuals.size()) {
  }

  inline bool get(individual_t const& individual) const noexcept {
    return flags_[to_index(individual)];
  }

  inline void set(individual_t& individual) noexcept {
    flags_[to_index(individual)] = true;
  }

  inline auto const& flags() const noexcept {
    return flags_;
  }

private:
  inline std::size_t to_index(individual_t const& individual) const noexcept {
    return &individual - first_;
  }

private:
  individual_t const* first_;
  std::vector<bool> flags_;
};

TEST(pareto_identify_tests, add_dominant) {
  // arrange
  std::vector<individual_t> individuals{{1, 0}, {0, 1}, {0, 0}};
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(
      std::ranges::subrange{std::ranges::begin(individuals),
                            std::ranges::begin(individuals) + 2},
      std::ranges::subrange{std::ranges::begin(individuals) + 2,
                            std::ranges::end(individuals)},
      t,
      cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(true, true, false));
}

TEST(pareto_identify_tests, add_dominated) {
  // arrange
  std::vector<individual_t> individuals{{1, 0}, {0, 1}, {1, 1}};
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(
      std::ranges::subrange{std::ranges::begin(individuals),
                            std::ranges::begin(individuals) + 2},
      std::ranges::subrange{std::ranges::begin(individuals) + 2,
                            std::ranges::end(individuals)},
      t,
      cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(false, false, true));
}

TEST(pareto_identify_tests, add_dominant_dominated) {
  // arrange
  std::vector<individual_t> individuals{{1, 0}, {0, 1}, {0, 0}, {1, 1}};
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(
      std::ranges::subrange{std::ranges::begin(individuals),
                            std::ranges::begin(individuals) + 2},
      std::ranges::subrange{std::ranges::begin(individuals) + 2,
                            std::ranges::end(individuals)},
      t,
      cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(true, true, false, true));
}
