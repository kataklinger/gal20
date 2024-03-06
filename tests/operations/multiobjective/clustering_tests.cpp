
#include "clustering.hpp"

#include "ranking.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tests::clustering {

using cmp_t = gal::dominate<std::less<>>;

using fitness_t = std::array<double, 2>;

using tags_t =
    std::tuple<gal::frontier_level_t, gal::int_rank_t, gal::cluster_label>;

using population_t = gal::
    population<int, fitness_t, cmp_t, double, gal::disabled_comparator, tags_t>;

constexpr fitness_t f1a{0.1, 1.9};
constexpr fitness_t f1b{0.2, 1.8};
constexpr fitness_t f1c{0.3, 1.7};
constexpr fitness_t f1d{0.7, 1.3};
constexpr fitness_t f1e{0.8, 1.2};
constexpr fitness_t f1f{0.9, 1.1};
constexpr fitness_t f1g{1.1, 0.9};
constexpr fitness_t f1h{1.2, 0.8};
constexpr fitness_t f1i{1.3, 0.7};
constexpr fitness_t f1j{1.7, 0.3};
constexpr fitness_t f1k{1.8, 0.2};
constexpr fitness_t f1l{1.9, 0.1};

constexpr fitness_t f2a{1.1, 2.9};
constexpr fitness_t f2b{1.2, 2.8};
constexpr fitness_t f2c{1.3, 2.7};
constexpr fitness_t f2d{1.7, 2.3};
constexpr fitness_t f2e{1.8, 2.2};
constexpr fitness_t f2f{1.9, 2.1};
constexpr fitness_t f2g{2.1, 1.9};
constexpr fitness_t f2h{2.2, 1.8};
constexpr fitness_t f2i{2.3, 1.7};
constexpr fitness_t f2j{2.7, 1.3};
constexpr fitness_t f2k{2.8, 1.2};
constexpr fitness_t f2l{2.9, 1.1};

template<typename Population>
inline auto get_cluster_label(Population const& population, std::size_t index) {
  return gal::get_tag<gal::cluster_label>(population.individuals()[index]);
}

class linkage_base_tests : public ::testing::Test {
protected:
  void SetUp(std::size_t target_size) {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1l}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2l}, tags_t{0, 0, 0}}};

    population_ = std::make_unique<population_t>(
        cmp_t{}, gal::disabled_comparator{}, target_size, false);
    population_->insert(individuals);

    pareto_ = gal::rank::level{}(*population_, gal::pareto_preserved_tag);
  }

  std::unique_ptr<population_t> population_;
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
};

class linkage_archive_overflow_tests : public linkage_base_tests {
protected:
  void SetUp() override {
    linkage_base_tests::SetUp(4);
  }
};

TEST_F(linkage_archive_overflow_tests, clusters_count) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 4);
}

TEST_F(linkage_archive_overflow_tests, clusters_content) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  std::vector all(clusters.begin(), clusters.end());
  gal::cluster_set::cluster cluster{1, 3};

  EXPECT_THAT(all, ::testing::Each(cluster));
}

TEST_F(linkage_archive_overflow_tests, clusters_labels) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 12; ++i) {
    EXPECT_EQ(get_cluster_label(*population_, i).index(), i / 3);
  }

  for (std::size_t i = 12; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unassigned());
  }
}

class linkage_archive_exact_tests : public linkage_base_tests {
protected:
  void SetUp() override {
    linkage_base_tests::SetUp(12);
  }
};

TEST_F(linkage_archive_exact_tests, clusters_count) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(linkage_archive_exact_tests, clusters_labels) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 12; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }

  for (std::size_t i = 12; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unassigned());
  }
}

class linkage_archive_underflow_pratial_tests : public linkage_base_tests {
protected:
  void SetUp() override {
    linkage_base_tests::SetUp(16);
  }
};

TEST_F(linkage_archive_underflow_pratial_tests, clusters_count) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 4);
}

TEST_F(linkage_archive_underflow_pratial_tests, clusters_content) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  std::vector all(clusters.begin(), clusters.end());
  gal::cluster_set::cluster cluster{2, 3};

  EXPECT_THAT(all, ::testing::Each(cluster));
}

TEST_F(linkage_archive_underflow_pratial_tests, clusters_labels) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 12; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }

  for (std::size_t i = 12; i < 24; ++i) {
    EXPECT_EQ(get_cluster_label(*population_, i).index(), (i - 12) / 3);
  }
}

class linkage_archive_underflow_full_tests : public linkage_base_tests {
protected:
  void SetUp() override {
    linkage_base_tests::SetUp(24);
  }
};

TEST_F(linkage_archive_underflow_full_tests, clusters_count) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(linkage_archive_underflow_full_tests, clusters_labels) {
  // arrange
  gal::cluster::linkage op{};

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }
}

class hypergird_base_tests : public ::testing::Test {
protected:
  void SetUp(std::size_t target_size) {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1l}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2l}, tags_t{0, 0, 0}}};

    population_ = std::make_unique<population_t>(
        cmp_t{}, gal::disabled_comparator{}, target_size, false);
    population_->insert(individuals);

    pareto_ = gal::rank::level{}(*population_, gal::pareto_preserved_tag);
  }

  std::unique_ptr<population_t> population_;
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
};

class hypergird_exact_tests : public hypergird_base_tests {
protected:
  void SetUp() override {
    hypergird_base_tests::SetUp(24);
  }
};

TEST_F(hypergird_exact_tests, grouped_clusters_count) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.5_dc, 0.5_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 8);
}

TEST_F(hypergird_exact_tests, grouped_clusters_content) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.5_dc, 0.5_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  std::vector all(clusters.begin(), clusters.end());
  gal::cluster_set::cluster cluster1{1, 3};
  gal::cluster_set::cluster cluster2{2, 3};

  EXPECT_THAT(all, ::testing::Contains(cluster1).Times(4));
  EXPECT_THAT(all, ::testing::Contains(cluster2).Times(4));
}

TEST_F(hypergird_exact_tests, grouped_clusters_labels) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.5_dc, 0.5_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 21; i += 3) {
    EXPECT_EQ(get_cluster_label(*population_, i + 0).index(),
              get_cluster_label(*population_, i + 1).index());
    EXPECT_EQ(get_cluster_label(*population_, i + 1).index(),
              get_cluster_label(*population_, i + 2).index());
  }
}

TEST_F(hypergird_exact_tests, unique_clusters_count) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.05_dc, 0.05_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(hypergird_exact_tests, unique_clusters_labels) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.05_dc, 0.05_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }
}

class hypergird_overflow_tests : public hypergird_base_tests {
protected:
  void SetUp() override {
    hypergird_base_tests::SetUp(12);
  }
};

TEST_F(hypergird_overflow_tests, unique_clusters_count) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.05_dc, 0.05_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(hypergird_overflow_tests, unique_clusters_labels) {
  using namespace gal::literals;

  // arrange
  gal::cluster::hypergrid<fitness_t, 0.05_dc, 0.05_dc> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 12; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }

  for (std::size_t i = 12; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unassigned());
  }
}

// ------

class adaptive_hypergird_base_tests : public ::testing::Test {
protected:
  void SetUp(std::size_t target_size) {
    using individual_t = population_t::individual_t;
    using evaluation_t = individual_t::evaluation_t;

    std::vector<individual_t> individuals{
        {0, evaluation_t{f1a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f1l}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2a}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2b}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2c}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2d}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2e}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2f}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2g}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2h}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2i}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2j}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2k}, tags_t{0, 0, 0}},
        {0, evaluation_t{f2l}, tags_t{0, 0, 0}}};

    population_ = std::make_unique<population_t>(
        cmp_t{}, gal::disabled_comparator{}, target_size, false);
    population_->insert(individuals);

    pareto_ = gal::rank::level{}(*population_, gal::pareto_preserved_tag);
  }

  std::unique_ptr<population_t> population_;
  gal::population_pareto_t<population_t, gal::pareto_preserved_t> pareto_{0};
};

class adaptive_hypergird_exact_tests : public adaptive_hypergird_base_tests {
protected:
  void SetUp() override {
    adaptive_hypergird_base_tests::SetUp(24);
  }
};

TEST_F(adaptive_hypergird_exact_tests, grouped_clusters_count) {
  // arrange
  gal::cluster::adaptive_hypergrid<6, 6> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 8);
}

TEST_F(adaptive_hypergird_exact_tests, grouped_clusters_content) {
  // arrange
  gal::cluster::adaptive_hypergrid<6, 6> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  std::vector all(clusters.begin(), clusters.end());
  gal::cluster_set::cluster cluster1{1, 3};
  gal::cluster_set::cluster cluster2{2, 3};

  EXPECT_THAT(all, ::testing::Contains(cluster1).Times(4));
  EXPECT_THAT(all, ::testing::Contains(cluster2).Times(4));
}

TEST_F(adaptive_hypergird_exact_tests, grouped_clusters_labels) {
  // arrange
  gal::cluster::adaptive_hypergrid<6, 6> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 21; i += 3) {
    EXPECT_EQ(get_cluster_label(*population_, i + 0).index(),
              get_cluster_label(*population_, i + 1).index());
    EXPECT_EQ(get_cluster_label(*population_, i + 1).index(),
              get_cluster_label(*population_, i + 2).index());
  }
}

TEST_F(adaptive_hypergird_exact_tests, unique_clusters_count) {
  // arrange
  gal::cluster::adaptive_hypergrid<60, 60> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(adaptive_hypergird_exact_tests, unique_clusters_labels) {
  // arrange
  gal::cluster::adaptive_hypergrid<60, 60> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }
}

class adaptive_hypergird_overflow_tests : public adaptive_hypergird_base_tests {
protected:
  void SetUp() override {
    adaptive_hypergird_base_tests::SetUp(12);
  }
};

TEST_F(adaptive_hypergird_overflow_tests, unique_clusters_count) {
  // arrange
  gal::cluster::adaptive_hypergrid<60, 60> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  EXPECT_EQ(clusters.size(), 0);
}

TEST_F(adaptive_hypergird_overflow_tests, unique_clusters_labels) {
  // arrange
  gal::cluster::adaptive_hypergrid<60, 60> op;

  // act
  auto clusters = op(*population_, pareto_);

  // assert
  for (std::size_t i = 0; i < 12; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unique());
  }

  for (std::size_t i = 12; i < 24; ++i) {
    EXPECT_TRUE(get_cluster_label(*population_, i).is_unassigned());
  }
}

} // namespace tests::clustering
