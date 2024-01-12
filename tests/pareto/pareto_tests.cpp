
#include "fitness.hpp"
#include "pareto.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

using individual_t = std::array<int, 2>;

template<std::ranges::range R>
constexpr auto to_vector(R&& r) {
  auto extracted =
      r | std::views::transform([](auto const& s) { return s.individual(); });

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
    individuals_.push_back(f1a);
    individuals_.push_back(f2a);
    individuals_.push_back(f2b);
    individuals_.push_back(f3a);
  }

  std::vector<individual_t> individuals_;
};

TEST_F(pareto_sort_tests, pareto_front_count) {
  // arrange
  gal::dominate cmp{std::less{}};

  // act
  auto sorted = individuals_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 3);
}

TEST_F(pareto_sort_tests, pareto_front_ordering) {
  // arrange
  gal::dominate cmp{std::less{}};

  // act
  auto sorted = individuals_ | gal::pareto::views::sort(cmp);

  // assert
  auto it = std::ranges::begin(sorted);
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f1a));

  ++it;
  EXPECT_THAT(to_vector(it->members()),
              ::testing::UnorderedElementsAre(f2a, f2b));
  ++it;
  EXPECT_THAT(to_vector(it->members()), ::testing::ElementsAre(f3a));
}
