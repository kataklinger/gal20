
#pragma once

#include "multiobjective.hpp"
#include "pareto.hpp"

namespace gal {
namespace rank {

  struct pareto_preserved_t {};
  inline constexpr pareto_preserved_t pareto_preserved{};

  struct pareto_reduced_t {};
  inline constexpr pareto_reduced_t pareto_reduced{};

  struct pareto_nondominated_t {};
  inline constexpr pareto_nondominated_t pareto_nondominated{};

  struct pareto_erased_t {};
  inline constexpr pareto_erased_t pareto_erased{};

  namespace details {

    template<typename RankTag, typename Individual>
    inline auto& get(Individual& individual) noexcept {
      return std::get<RankTag>(individual.tags());
    }

    template<typename RankTag, typename Individual>
    inline auto const& get(Individual const& individual) noexcept {
      return std::get<RankTag>(individual.tags());
    }

    template<typename RankTag, ranked_population<RankTag> Population>
    inline void clean_tag(Population& population) {
      for (auto&& individual : population.individuals()) {
        get<RankTag>(individual) = {};
      }
    }

    template<typename Population>
    using population_pareto = pareto_sets<typename Population::individual_t>;

    template<typename Population, typename Preserve>
    class wrapped_pareto;

    template<typename Population>
    class wrapped_pareto<Population, pareto_preserved_t> {
    public:
      using base_pareto_t = population_pareto<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_pareto(std::size_t max_individuals)
          : base_{max_individuals} {
      }

      inline void add_inidividual(individual_t& individual) {
        base_.add_inidividual(individual);
      }

      inline void next() {
        base_.next();
      }

      inline auto extract() noexcept {
        return std::move(base_);
      }

    private:
      base_pareto_t base_;
    };

    template<typename Population>
    class wrapped_pareto<Population, pareto_reduced_t> {
    public:
      using base_pareto_t = population_pareto<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_pareto(std::size_t max_individuals)
          : base_{max_individuals} {
      }

      inline void add_inidividual(individual_t& individual) {
        base_.add_inidividual(individual);
      }

      inline void next() {
        if (base_.count() == 1) {
          base_.next();
        }
      }

      inline auto extract() noexcept {
        return std::move(base_);
      }

    private:
      base_pareto_t base_;
    };

    template<typename Population>
    class wrapped_pareto<Population, pareto_nondominated_t> {
    public:
      using base_pareto_t = population_pareto<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_pareto(std::size_t max_individuals)
          : base_{max_individuals} {
      }

      inline void add_inidividual(individual_t& individual) {
        if (!done_) {
          base_.add_inidividual(individual);
        }
      }

      inline void next() {
        done_ = true;
      }

      inline auto extract() noexcept {
        return std::move(base_);
      }

    private:
      bool done_{};
      base_pareto_t base_;
    };

    template<typename Population>
    class wrapped_pareto<Population, pareto_erased_t> {
      using individual_t = typename Population::individual_t;

      inline explicit wrapped_pareto(std::size_t /*unused*/) {
      }

      inline void add_inidividual(individual_t& individual) noexcept {
      }

      inline void next() noexcept {
      }

      inline auto unwrap() noexcept {
        return population_pareto<Population>{};
      }
    };

    template<typename Population>
    inline void populate_binary_pareto(Population& population,
                                       population_pareto<Population>& output,
                                       binary_rank which) {
      for (auto&& individual : population.individuals()) {
        if (get<bin_rank_t>(individual) == which) {
          output.add_individuals(individual);
        }
      }

      output.next();
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_reduced_t /*unused*/) {
      population_pareto<Population> output{population.size()};

      populate_binary_pareto(population, output, binary_rank::nondominated);
      populate_binary_pareto(population, output, binary_rank::dominated);

      return output;
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_nondominated_t /*unused*/) {
      population_pareto<Population> output{population.size()};

      populate_binary_pareto(population, output, binary_rank::nondominated);

      return output;
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_erased_t /*unused*/) {
      return population_pareto<Population>{};
    }

  } // namespace details

  // non-dominated vs dominated (pesa, pesa-ii, paes)
  class binary {
  private:
    struct tracker {
      template<typename Individual>
      inline bool get(Individual const& individual) const noexcept {
        return details::get<bin_rank_t>(individual) == binary_rank::dominated;
      }

      template<typename Individual>
      inline void set(Individual& individual) const noexcept {
        details::get<bin_rank_t>(individual) = binary_rank::dominated;
      }
    };

  public:
    template<ranked_population<bin_rank_t> Population>
    auto operator()(Population& population,
                    pareto_preserved_t /*unused*/) const {
      details::clean_tag<bin_rank_t>(population);

      details::population_pareto<Population> output{population.size()};
      auto current = binary_rank::nondominated;

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& ind = solution.individual();
          details::get<bin_rank_t>(ind) = current;

          output.add_inidividual(ind);
        }

        current = binary_rank::dominated;
        output.next();
      }

      return output;
    }

    template<ranked_population<bin_rank_t> Population, typename Pareto>
    void operator()(Population& population, Pareto preserve) const {
      std::vector<typename Population::individual_t> ranked, nonranked;
      std::ranges::partition_copy(population.individuals(),
                                  std::back_inserter(ranked),
                                  std::back_inserter(nonranked),
                                  [](auto const& ind) {
                                    return details::get<bin_rank_t>(ind) !=
                                           binary_rank::undefined;
                                  });

      pareto::identify_dominated(
          ranked, nonranked, tracker{}, population.raw_comparator());

      return details::generate_binary_pareto(population, preserve);
    }
  };

  // pareto front level (nsga & nsga-ii)
  class level {
  public:
    template<ranked_population<int_rank_t> Population, typename Pareto>
    auto operator()(Population& population, Pareto /*unused*/) const {
      details::clean_tag<int_rank_t>(population);

      details::wrapped_pareto<Population, Pareto> output{population.size()};

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& ind = solution.individual();
          details::get<int_rank_t>(ind) = frontier.level();

          output.add_individual(ind);
        }

        output.next();
      }

      return output.extract();
    }
  };

  namespace details {

    template<typename Population, typename RankTag>
    inline auto prepare_strength_fast(Population& pop) {
      clean_tag<RankTag>(pop);
      return pareto::analyze(pop.indviduals(), pop.raw_comparator());
    }

    template<typename Population, typename RankTag>
    inline auto prepare_strength_slow(Population& pop) {
      clean_tag<RankTag>(pop);
      return population_pareto<Population>{pop.size()};
    }

    template<typename Solutions, typename Pareto>
    inline void populate_strength_pareto(Solutions& solutions,
                                         Pareto& output,
                                         bool which) {
      for (auto&& solution : solutions) {
        if (solution.nondominated() == which) {
          output.add_individuals(solution.individual());
        }
      }

      output.next();
    }

    template<typename Solutions>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         std::size_t size,
                                         pareto_reduced_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      pareto_sets<individual_t> output{size};

      populate_strength_pareto(solutions, output, true);
      populate_strength_pareto(solutions, output, false);

      return output;
    }

    template<typename Solutions>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         std::size_t size,
                                         pareto_nondominated_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      pareto_sets<individual_t> output{size};

      populate_strength_pareto(solutions, output, true);

      return output;
    }

    template<typename Solutions>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         std::size_t /*unused*/,
                                         pareto_erased_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      return pareto_sets<individual_t>{};
    }

  } // namespace details

  // accumulated pareto level (rdga)
  class accumulated_level {
  public:
    template<ranked_population<int_rank_t> Population, typename Pareto>
    void operator()(Population& population, Pareto /*unused*/) const {
      details::clean_tag<int_rank_t>(population);

      details::wrapped_pareto<Population, Pareto> output{population.size()};

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& ind = solution.individual();

          details::get<int_rank_t>(ind) += 1;

          for (auto&& dominated : solution.dominated()) {
            details::get<int_rank_t>(dominated.individual()) +=
                details::get<int_rank_t>(ind);
          }

          output.add_individual(ind);
        }

        output.next();
      }

      return output.extract();
    }
  };

  // number of dominated, first front (spea)
  class strength {
  public:
    template<ranked_population<real_rank_t> Population>
    auto operator()(Population& population,
                    pareto_preserved_t /*unused*/) const {
      auto output = details::prepare_strength_slow(population);

      auto sorted = population.indviduals() |
                    pareto::views::sort(population.raw_comparator());

      auto first = std::ranges::begin(sorted);

      auto dominated_count =
          population.size() - std::ranges::size(first->members());

      for (auto&& solution : first->members()) {
        assign_nondominated_strength(solution, dominated_count);

        output.add_inidividual(solution.individual());
      }

      output.next();

      for (auto&& frontier : std::ranges::subrange{std::ranges::next(first),
                                                   std::ranges::end(sorted)}) {
        for (auto&& solution : frontier.members()) {
          assign_dominated_strength(solution);

          output.add_inidividual(solution.individual());
        }

        output.next();
      }

      return output;
    }

    template<ranked_population<real_rank_t> Population, typename Pareto>
    auto operator()(Population& population, Pareto preserved) const {
      auto analyzed = details::prepare_strength_fast<real_rank_t>(population);

      auto dominated_count = std::ranges::count_if(
          analyzed, [](auto& solution) { return !solution.nondominated(); });

      for (auto&& solution : analyzed) {
        if (solution.nondominated()) {
          assign_nondominated_strength(solution, dominated_count);
        }
        else {
          assign_dominated_strength(solution);
        }
      }

      return details::generate_strength_pareto(
          analyzed, population.size(), preserved);
    }

  private:
    template<typename Solution>
    inline static void
        assign_nondominated_strength(Solution& solution,
                                     std::size_t dominated_count) noexcept {
      auto s =
          solution.dominated_total() / static_cast<double>(dominated_count);
      details::get<real_rank_t>(solution.individual()) = s;

      for (auto&& dominated : solution.dominated()) {
        details::get<real_rank_t>(dominated.individual()) += s;
      }
    };

    template<typename Solution>
    inline static void assign_dominated_strength(Solution& solution) noexcept {
      details::get<real_rank_t>(solution.individual()) += 1.0;
    };
  };

  // number of dominated, all fronts (spea-ii)
  class accumulated_strength {
  public:
    template<ranked_population<int_rank_t> Population>
    auto operator()(Population& population,
                    pareto_preserved_t /*unused*/) const {
      auto output = details::prepare_strength_slow(population);

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          assign_strength(solution);

          output.add_inidividual(solution.individual());
        }

        output.next();
      }

      return output;
    }

    template<ranked_population<int_rank_t> Population, typename Pareto>
    auto operator()(Population& population, Pareto preserved) const {
      auto analyzed = details::prepare_strength_fast<int_rank_t>(population);

      for (auto&& solution : analyzed) {
        assign_strength(solution);
      }

      return details::generate_strength_pareto(
          analyzed, population.size(), preserved);
    }

  private:
    template<typename Solution>
    inline static void assign_strength(Solution& solution) noexcept {
      auto s = solution.dominated_total();

      for (auto&& dominated : solution.dominated()) {
        details::get<int_rank_t>(dominated.individual()) += s;
      }
    }
  };

} // namespace rank
} // namespace gal
