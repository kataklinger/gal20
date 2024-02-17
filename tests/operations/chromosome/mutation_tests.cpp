
#include <mutation.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::mutation {

using chromosome_vector_t = std::vector<int>;
using chromosome_list_t = std::list<int>;

class defined_random_sequence {
public:
  inline defined_random_sequence(std::initializer_list<std::size_t> seq)
      : seq_{seq} {
  }

  inline auto next() noexcept {
    return seq_[current_++];
  }

private:
  std::vector<std::size_t> seq_;
  std::size_t current_{0};
};

struct random_index_adapter {
  inline auto operator()() noexcept {
    return seq_->next();
  }

  defined_random_sequence* seq_;
};

struct deterministic_index_generator {
public:
  inline deterministic_index_generator(std::initializer_list<std::size_t> seq)
      : seq_{seq} {
  }

  inline auto operator()() const noexcept {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t min_idx,
                         std::ranges::sized_range auto const& range) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::ranges::sized_range auto const& range) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t min_idx, std::size_t max_idx) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t max_idx) const {
    return random_index_adapter{&seq_};
  }

private:
  mutable defined_random_sequence seq_;
};

struct mutation_tests : public ::testing::Test {
protected:
  inline static constexpr std::array original_{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  void SetUp() override {
    chromosome_vector_ =
        chromosome_vector_t{original_.begin(), original_.end()};
    chromosome_list_ = chromosome_list_t{original_.begin(), original_.end()};
  }

  chromosome_vector_t chromosome_vector_;
  chromosome_list_t chromosome_list_;
};

TEST_F(mutation_tests, interchange_vector_one_pair) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::interchange op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 8, 2, 3, 4, 5, 6, 7, 1, 9));
}

TEST_F(mutation_tests, interchange_vector_two_pairs) {
  // arrange
  deterministic_index_generator rng_{1, 8, 0, 9};
  gal::mutate::interchange op{rng_, gal::countable<2>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(9, 8, 2, 3, 4, 5, 6, 7, 1, 0));
}

TEST_F(mutation_tests, interchange_list_one_pair) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::interchange op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 8, 2, 3, 4, 5, 6, 7, 1, 9));
}

TEST_F(mutation_tests, interchange_list_two_pairs) {
  // arrange
  deterministic_index_generator rng_{1, 8, 0, 9};
  gal::mutate::interchange op{rng_, gal::countable<2>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(9, 8, 2, 3, 4, 5, 6, 7, 1, 0));
}

TEST_F(mutation_tests, suffle_vector_one_forward) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 1, 9));
}

TEST_F(mutation_tests, suffle_vector_one_backward) {
  // arrange
  deterministic_index_generator rng{8, 1};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(0, 8, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, suffle_vector_two) {
  // arrange
  deterministic_index_generator rng{0, 9, 8, 0};
  gal::mutate::shuffle op{rng, gal::countable<2>};

  // act
  op(chromosome_vector_);

  // assert
  EXPECT_THAT(chromosome_vector_,
              ::testing::ElementsAre(9, 1, 2, 3, 4, 5, 6, 7, 8, 0));
}

//////////

TEST_F(mutation_tests, suffle_list_one_forward) {
  // arrange
  deterministic_index_generator rng{1, 8};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 1, 9));
}

TEST_F(mutation_tests, suffle_list_one_backward) {
  // arrange
  deterministic_index_generator rng{8, 1};
  gal::mutate::shuffle op{rng, gal::countable<1>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(0, 8, 1, 2, 3, 4, 5, 6, 7, 9));
}

TEST_F(mutation_tests, suffle_list_two) {
  // arrange
  deterministic_index_generator rng{0, 9, 8, 0};
  gal::mutate::shuffle op{rng, gal::countable<2>};

  // act
  op(chromosome_list_);

  // assert
  EXPECT_THAT(chromosome_list_,
              ::testing::ElementsAre(9, 1, 2, 3, 4, 5, 6, 7, 8, 0));
}

} // namespace tests::mutation
