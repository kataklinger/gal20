
#pragma once

#include "pareto.hpp"
#include "population.hpp"

namespace gal {
namespace rank {

  template<typename Rank>
  concept ranking = std::regular<Rank> && std::totally_ordered<Rank>;

  template<ranking Value>
  class rank {
  public:
    using value_t = Value;

  public:
    inline rank(value_t value) noexcept
        : value_{value} {
    }

    inline operator value_t() const noexcept {
      return value_;
    }

    inline value_t get() const noexcept {
      return value_;
    }

  private:
    value_t value_;
  };

  enum class binary_rank : std::uint8_t { nondominated, dominated, undefined };

  inline auto operator<=>(binary_rank lhs, binary_rank rhs) noexcept {
    using type = std::underlying_type_t<binary_rank>;
    return static_cast<type>(lhs) <=> static_cast<type>(rhs);
  }

  using bin_rank_t = rank<binary_rank>;
  using int_rank_t = rank<std::size_t>;
  using fp_rank_t = rank<double>;

  template<typename Population, typename Rank>
  concept ranked_population =
      multiobjective_population<Population> && tagged_with<Population, Rank>;

  namespace details {

    template<typename Rank, typename Individual>
    inline auto& get(Individual& individual) noexcept {
      return std::get<Rank>(individual.tags());
    }

    template<typename Rank, typename Individual>
    inline auto const& get(Individual const& individual) noexcept {
      return std::get<Rank>(individual.tags());
    }

    template<typename Rank, ranked_population<Rank> Population>
    inline void clean(Population& population) {
      for (auto& individual : population.individuals()) {
        get<Rank>(individual) = {};
      }
    }

  } // namespace details

  class binary {
  private:
    struct tracker {
      template<typename Individual>
      inline bool get(Individual const& individual) const noexcept {
        return details::get<bin_rank_t>(individual);
      }

      template<typename Individual>
      inline void set(Individual& individual) const noexcept {
        details::get<bin_rank_t>(individual) = true;
      }
    };

  public:
    template<ranked_population<bin_rank_t> Population>
    void operator()(Population& population) const {
      using individual_t = typename Population::individual_t;

      std::vector<individual_t> ranked, nonranked;
      std::ranges::partition_copy(population.individuals(),
                                  std::back_inserter(ranked),
                                  std::back_inserter(nonranked),
                                  [](individual_t const& ind) {
                                    return details::get<bin_rank_t>(ind) !=
                                           binary_rank::undefined;
                                  });

      pareto::identify_dominated(
          ranked, nonranked, tracker{}, population.raw_comparator());
    }
  };

  class level {
  public:
    template<ranked_population<int_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<int_rank_t>(population);

      for (auto& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto& individual : frontier.members()) {
          details::get<int_rank_t>(individual) = frontier.level();
        }
      }
    }
  };

  class strength {
  public:
    template<ranked_population<fp_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<fp_rank_t>(population);

      auto analized =
          pareto::analyze(population.indviduals(), population.raw_comparator());

      auto total_count = static_cast<double>(
          std::ranges::count_if(analized, [](auto& individual) {
            return !individual.nondominated();
          }));

      for (auto& individual : analized) {
        if (individual.nondominated()) {
          auto s = individual.dominated_total() / div;
          details::get<fp_rank_t>(individual) = s;
          for (auto& dominated : individual.dominated()) {
            details::get<fp_rank_t>(dominated) += s;
          }
        }
        else {
          details::get<fp_rank_t>(individual) += 1.0;
        }
      }
    }
  };

  class acc_strength {
  public:
    template<ranked_population<int_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<int_rank_t>(population);

      for (auto& individual : pareto::analyze(population.indviduals(),
                                              population.raw_comparator())) {
        auto s = individual.dominated_total();
        for (auto& dominated : individual.dominated()) {
          details::get<int_rank_t>(dominated) += s;
        }
      }
    }
  };

  class acc_level {
  public:
    template<ranked_population<int_rank_t> Population>
    void operator()(Population& population) const {
      details::clean<int_rank_t>(population);

      for (auto& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto& individual : frontier.members()) {
          details::get<int_rank_t>(individual) += 1;

          for (auto& dominated : individual.dominated()) {
            details::get<int_rank_t>(dominated) +=
                details::get<int_rank_t>(individual);
          }
        }
      }
    }
  };

} // namespace rank
} // namespace gal
