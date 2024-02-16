
#include <mutation.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::mutation {

using chromosome_t = std::vector<int>;

template<typename Ty>
class rng {

public:
  rng(std::initializer_list<Ty> seq)
      : seq_{seq_} {
  }

  inline auto operator()() noexcept {
    return seq_[current_++];
  }

private:
  std::vector<Ty> seq_;
  std::size_t current_;
};

struct mutation_tests : public ::testing::Test {
protected:
  void SetUp() override {
    chromosome_ = chromosome_t{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  }

  chromosome_t chromosome_;
};

TEST_F(mutation_tests, interchange) {
}

} // namespace tests::mutation
