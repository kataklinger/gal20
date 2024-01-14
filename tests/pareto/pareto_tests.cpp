
#include "fitness.hpp"
#include "pareto.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

using individual_t = std::array<int, 2>;

static gal::dominate cmp{std::less{}};

static constexpr individual_t f1a{0, 0};
static constexpr individual_t f2a{1, 0};
static constexpr individual_t f2b{0, 1};
static constexpr individual_t f3a{1, 1};

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
  void SetUp() override {
  }

  std::vector<individual_t> individuals_0_{0};
  std::vector<individual_t> individuals_1_{f1a};
  std::vector<individual_t> individuals_5_{f1a, f2a, f2b, f3a};
};

TEST_F(pareto_sort_tests, pareto_sort_front_count_multi) {
  // arrange
  gal::dominate cmp{std::less{}};

  // act
  auto sorted = individuals_5_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 3);
}

TEST_F(pareto_sort_tests, pareto_sort_front_ordering_multi) {
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

TEST_F(pareto_sort_tests, pareto_sort_front_count_single) {
  // arrange

  // act
  auto sorted = individuals_1_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 1);
}

TEST_F(pareto_sort_tests, pareto_sort_front_ordering_single) {
  // arrange

  // act
  auto sorted = individuals_1_ | gal::pareto::views::sort(cmp);

  // assert
  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f1a));
}

TEST_F(pareto_sort_tests, pareto_sort_front_count_empty) {
  // arrange

  // act
  auto sorted = individuals_0_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 1);

  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::IsEmpty());
}

TEST_F(pareto_sort_tests, pareto_analyze_multi) {
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

TEST_F(pareto_sort_tests, pareto_analyze_single) {
  // arrange

  // act
  auto nondominated = gal::pareto::analyze(individuals_1_, cmp);

  //   assert
  auto it = std::ranges::begin(nondominated);
  auto indv = *it;
  EXPECT_TRUE(indv.nondominated());
  EXPECT_THAT(to_vector(indv.dominated()), ::testing::IsEmpty());
}

TEST_F(pareto_sort_tests, pareto_analyze_empty) {
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

inline auto split_range(std::vector<individual_t>& individuals,
                        std::size_t where) noexcept {
  return std::tuple{
      std::ranges::subrange{std::ranges::begin(individuals),
                            std::ranges::begin(individuals) + where},
      std::ranges::subrange{std::ranges::begin(individuals) + where,
                            std::ranges::end(individuals)}};
}

TEST(pareto_identify_tests, add_dominant) {
  // arrange
  std::vector<individual_t> individuals{f2a, f2b, f1a};
  auto [existing, added] = split_range(individuals, 2);
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(existing, added, t, cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(true, true, false));
}

TEST(pareto_identify_tests, add_dominated) {
  // arrange
  std::vector<individual_t> individuals{f2a, f2b, f3a};
  auto [existing, added] = split_range(individuals, 2);
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(existing, added, t, cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(false, false, true));
}

TEST(pareto_identify_tests, add_dominant_dominated) {
  // arrange
  std::vector<individual_t> individuals{f2a, f2b, f1a, f3a};
  auto [existing, added] = split_range(individuals, 2);
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(existing, added, t, cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(true, true, false, true));
}

TEST(pareto_identify_tests, add_nondominant_nondominated) {
  // arrange
  std::vector<individual_t> individuals{f2a, f2b};
  auto [existing, added] = split_range(individuals, 1);
  tracker t{individuals};

  // act
  gal::pareto::identify_dominated(existing, added, t, cmp);

  //   assert
  EXPECT_THAT(t.flags(), ::testing::ElementsAre(false, false));
}
