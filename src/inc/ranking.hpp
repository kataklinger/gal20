
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace rank {
  namespace details {

    template<typename Population, typename Preserve>
    class wrapped_pareto;

    template<typename Population>
    class wrapped_pareto<Population, pareto_preserved_t> {
    public:
      using base_pareto_t = population_pareto_t<Population>;
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
      using base_pareto_t = population_pareto_t<Population>;
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
      using base_pareto_t = population_pareto_t<Population>;
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
        return population_pareto_t<Population>{};
      }
    };

    template<typename Population>
    inline void populate_binary_pareto(Population& population,
                                       population_pareto_t<Population>& output,
                                       binary_rank which) {
      auto front_level = output.count() + 1;

      for (auto&& individual : population.individuals()) {
        if (get<bin_rank_t>(individual) == which) {
          output.add_individuals(individual);
        }

        get<frontier_level_t>(individual) = front_level;
      }

      output.next();
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_reduced_t /*unused*/) {
      population_pareto_t<Population> output{population.size()};
      populate_binary_pareto(population, output, binary_rank::nondominated);
      populate_binary_pareto(population, output, binary_rank::dominated);

      return output;
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_nondominated_t /*unused*/) {
      clean_tags<frontier_level_t>(population);

      population_pareto_t<Population> output{population.size()};
      populate_binary_pareto(population, output, binary_rank::nondominated);

      return output;
    }

    template<typename Population>
    inline auto generate_binary_pareto(Population& population,
                                       pareto_erased_t /*unused*/) {
      clean_tags<frontier_level_t>(population);

      return population_pareto_t<Population>{};
    }

  } // namespace details

  // non-dominated vs dominated (pesa, pesa-ii, paes)
  class binary {
  private:
    struct tracker {
      template<typename Individual>
      inline bool get(Individual const& individual) const noexcept {
        return get_tag<bin_rank_t>(individual) == binary_rank::dominated;
      }

      template<typename Individual>
      inline void set(Individual& individual) const noexcept {
        get_tag<bin_rank_t>(individual) = binary_rank::dominated;
      }
    };

  public:
    template<ranked_population<bin_rank_t> Population>
    auto operator()(Population& population,
                    pareto_preserved_t /*unused*/) const {
      clean_tags<bin_rank_t>(population);

      population_pareto_t<Population> output{population.size()};

      auto current = binary_rank::nondominated;
      pareto::frontier_level front_level = 1;

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& individual = solution.individual();

          get_tag<bin_rank_t>(individual) = current;
          get_tag<frontier_level_t>(individual) = front_level;

          output.add_inidividual(individual);
        }

        current = binary_rank::dominated;
        front_level = 2;

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
                                    return get_tag<bin_rank_t>(ind) !=
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
      clean_tags<int_rank_t>(population);

      details::wrapped_pareto<Population, Pareto> output{population.size()};

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& individual = solution.individual();

          get_tag<int_rank_t>(individual) = frontier.level();
          get_tag<frontier_level_t>(individual) = frontier.level();

          output.add_individual(individual);
        }

        output.next();
      }

      return output.extract();
    }
  };

  // accumulated pareto level (rdga)
  class accumulated_level {
  public:
    template<ranked_population<int_rank_t> Population, typename Pareto>
    void operator()(Population& population, Pareto /*unused*/) const {
      clean_tags<int_rank_t>(population);

      details::wrapped_pareto<Population, Pareto> output{population.size()};

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& individual = solution.individual();

          auto level = (get_tag<int_rank_t>(individual) += 1);
          get_tag<frontier_level_t>(individual) = frontier.level();

          for (auto&& dominated : solution.dominated()) {
            get_tag<int_rank_t>(dominated.individual()) += level;
          }

          output.add_individual(individual);
        }

        output.next();
      }

      return output.extract();
    }
  };

  namespace details {

    template<typename Population, typename RankTag>
    inline auto prepare_strength_fast(Population& pop) {
      clean_tags<RankTag>(pop);
      return pareto::analyze(pop.indviduals(), pop.raw_comparator());
    }

    template<typename Population, typename RankTag>
    inline auto prepare_strength_slow(Population& pop) {
      clean_tags<RankTag>(pop);
      return population_pareto_t<Population>{pop.size()};
    }

    template<typename Solutions, typename Pareto>
    inline void populate_strength_pareto(Solutions& solutions,
                                         Pareto& output,
                                         bool which) {
      auto front_level = output.count() + 1;

      for (auto&& solution : solutions) {
        auto& individual = solution.individual();

        if (solution.nondominated() == which) {
          output.add_individuals(individual);
        }

        get<frontier_level_t>(individual) = front_level;
      }

      output.next();
    }

    template<typename Solutions, typename Population>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         Population& population,
                                         pareto_reduced_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;

      pareto_sets<individual_t> output{population.size()};
      populate_strength_pareto(solutions, output, true);
      populate_strength_pareto(solutions, output, false);

      return output;
    }

    template<typename Solutions, typename Population>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         Population& population,
                                         pareto_nondominated_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;

      clean_tags<frontier_level_t>(population);

      pareto_sets<individual_t> output{population.size()};
      populate_strength_pareto(solutions, output, true);

      return output;
    }

    template<typename Solutions, typename Population>
    inline auto generate_strength_pareto(Solutions& solutions,
                                         Population& population,
                                         pareto_erased_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;

      clean_tags<frontier_level_t>(population);

      return pareto_sets<individual_t>{};
    }

  } // namespace details

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
        auto& individual = solution.individual();

        assign_nondominated_strength(solution, dominated_count);

        get_tag<frontier_level_t>(individual) = first->level();

        output.add_inidividual(individual);
      }

      output.next();

      for (auto&& frontier : std::ranges::subrange{std::ranges::next(first),
                                                   std::ranges::end(sorted)}) {
        for (auto&& solution : frontier.members()) {
          auto& individual = solution.individual();

          assign_dominated_strength(solution);

          get_tag<frontier_level_t>(individual) = frontier.level();

          output.add_inidividual(individual);
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

      return details::generate_strength_pareto(analyzed, population, preserved);
    }

  private:
    template<typename Solution>
    inline static void
        assign_nondominated_strength(Solution& solution,
                                     std::size_t dominated_count) noexcept {
      auto s =
          solution.dominated_total() / static_cast<double>(dominated_count);
      get_tag<real_rank_t>(solution.individual()) = s;

      for (auto&& dominated : solution.dominated()) {
        get_tag<real_rank_t>(dominated.individual()) += s;
      }
    };

    template<typename Solution>
    inline static void assign_dominated_strength(Solution& solution) noexcept {
      get_tag<real_rank_t>(solution.individual()) += 1.0;
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
          auto& individual = solution.individual();

          assign_strength(solution);

          get_tag<frontier_level_t>(individual) = frontier.level();

          output.add_inidividual(individual);
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

      return details::generate_strength_pareto(analyzed, population, preserved);
    }

  private:
    template<typename Solution>
    inline static void assign_strength(Solution& solution) noexcept {
      auto s = solution.dominated_total();

      for (auto&& dominated : solution.dominated()) {
        get_tag<int_rank_t>(dominated.individual()) += s;
      }
    }
  };

} // namespace rank
} // namespace gal
