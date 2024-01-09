
#include "fitness.hpp"
#include "pareto.hpp"

#include "gtest/gtest.h"

#include "array"
#include "vector"

class pareto_sort_tests : public testing::Test {
protected:
  void SetUp() override {
    individuals_.push_back({0, 0});
    individuals_.push_back({1, 0});
    individuals_.push_back({0, 1});
    individuals_.push_back({1, 1});
  }

  std::vector<std::array<int, 2>> individuals_;
};

TEST_F(pareto_sort_tests, pareto_front_count) {
  // arrange
  gal::dominate cmp{std::less{}};

  // act
  auto sorted = individuals_ | gal::pareto::views::sort(cmp);

  // assert
  EXPECT_EQ(std::ranges::distance(sorted), 3);
}
