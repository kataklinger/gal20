
#pragma once

#include "multiobjective.hpp"

namespace gal::rank {

namespace details {

  template<typename Population, typename Preserved>
  inline void
      populate_binary_pareto(Population& population,
                             population_pareto_t<Population, Preserved>& output,
                             binary_rank which) {
    auto front_level = static_cast<pareto::frontier_level>(which);

    for (auto&& individual : population.individuals()) {
      if (get_tag<bin_rank_t>(individual) == which) {
        output.add_individual(individual);
        get_tag<frontier_level_t>(individual) = front_level;
      }
    }

    output.next();
  }

  template<typename Population, typename Preserved>
  inline auto generate_binary_pareto(Population& population,
                                     Preserved /*unused*/) {
    population_pareto_t<Population, Preserved> output{
        population.current_size()};

    populate_binary_pareto(population, output, binary_rank::nondominated);
    populate_binary_pareto(population, output, binary_rank::dominated);

    output.finish();
    return output;
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
  auto operator()(Population& population, pareto_preserved_t /*unused*/) const {
    clean_tags<bin_rank_t>(population);

    population_pareto_t<Population, pareto_preserved_t> output{
        population.current_size()};

    auto current = binary_rank::nondominated;

    for (auto&& frontier :
         population.individuals() |
             pareto::views::sort(population.adopted_raw_comparator())) {
      for (auto&& solution : frontier.members()) {
        auto& individual = solution.individual();

        get_tag<bin_rank_t>(individual) = current;
        get_tag<frontier_level_t>(individual) =
            static_cast<pareto::frontier_level>(current);

        output.add_individual(individual);
      }

      if (std::exchange(current, binary_rank::dominated) ==
          binary_rank::nondominated) {
        output.next();
      }
    }

    output.next();
    output.finish();

    return output;
  }

  template<ranked_population<bin_rank_t> Population, typename Pareto>
  auto operator()(Population& population, Pareto preserve) const {
    auto nonranked =
        std::ranges::partition(population.individuals(), [](auto const& ind) {
          return get_tag<bin_rank_t>(ind) != binary_rank::undefined;
        });

    std::ranges::subrange ranked{std::ranges::begin(population.individuals()),
                                 nonranked.begin()};

    pareto::identify_dominated(
        ranked, nonranked, tracker{}, population.adopted_raw_comparator());

    for (auto&& individual : population.individuals()) {
      if (get_tag<bin_rank_t>(individual) == binary_rank::undefined) {
        get_tag<bin_rank_t>(individual) = binary_rank::nondominated;
      }
    }

    return details::generate_binary_pareto(population, preserve);
  }
};

// pareto front level (nsga & nsga-ii)
class level {
public:
  template<ranked_population<int_rank_t> Population, typename Pareto>
  auto operator()(Population& population, Pareto /*unused*/) const {
    clean_tags<int_rank_t>(population);

    population_pareto_t<Population, Pareto> output{population.current_size()};

    for (auto&& frontier :
         population.individuals() |
             pareto::views::sort(population.adopted_raw_comparator())) {
      for (auto&& solution : frontier.members()) {
        auto& individual = solution.individual();

        get_tag<int_rank_t>(individual) = frontier.level();
        get_tag<frontier_level_t>(individual) = frontier.level();

        output.add_individual(individual);
      }

      output.next();
    }

    output.finish();
    return output;
  }
};

// accumulated pareto level (rdga)
class accumulated_level {
public:
  template<ranked_population<int_rank_t> Population, typename Pareto>
  auto operator()(Population& population, Pareto /*unused*/) const {
    clean_tags<int_rank_t>(population);

    population_pareto_t<Population, Pareto> output{population.current_size()};

    for (auto&& frontier :
         population.individuals() |
             pareto::views::sort(population.adopted_raw_comparator())) {
      for (auto&& solution : frontier.members()) {
        auto& individual = solution.individual();

        std::size_t acc_level{(get_tag<int_rank_t>(individual) += 1)};
        get_tag<frontier_level_t>(individual) = frontier.level();

        for (auto&& dominated : solution.dominated()) {
          get_tag<int_rank_t>(dominated.individual()) += acc_level;
        }

        output.add_individual(individual);
      }

      output.next();
    }

    output.finish();
    return output;
  }
};

namespace details {

  template<typename RankTag, typename Population>
  inline auto prepare_strength_fast(Population& population) {
    clean_tags<RankTag>(population);
    return pareto::analyze(population.individuals(),
                           population.adopted_raw_comparator());
  }

  template<typename RankTag, typename Population>
  inline auto prepare_strength_slow(Population& population) {
    clean_tags<RankTag>(population);
    return population_pareto_t<Population, pareto_preserved_t>{
        population.current_size()};
  }

  template<typename Solutions, typename Pareto>
  inline void populate_strength_pareto(Solutions& solutions,
                                       Pareto& output,
                                       pareto::frontier_level front_level) {

    for (auto which = front_level < 2; auto&& solution : solutions) {
      auto& individual = solution.individual();

      if (solution.nondominated() == which) {
        output.add_individual(individual);
        get_tag<frontier_level_t>(individual) = front_level;
      }
    }

    output.next();
  }

  template<typename Solutions, typename Population, typename Pareto>
  inline auto generate_strength_pareto(Solutions& solutions,
                                       Population& population,
                                       Pareto /*unused*/) {
    using individual_t =
        typename std::ranges::range_value_t<Solutions>::individual_t;

    pareto_sets<individual_t, Pareto> output{population.current_size()};

    populate_strength_pareto(solutions, output, 1);
    populate_strength_pareto(solutions, output, 2);

    output.finish();
    return output;
  }

} // namespace details

// number of dominated, first front (spea)
class strength {
public:
  template<ranked_population<real_rank_t> Population>
  auto operator()(Population& population, pareto_preserved_t /*unused*/) const {
    auto output = details::prepare_strength_slow<real_rank_t>(population);

    auto sorted = population.individuals() |
                  pareto::views::sort(population.adopted_raw_comparator());

    auto first = std::ranges::begin(sorted);

    auto dominated_count =
        population.current_size() - std::ranges::size(first->members());

    for (auto&& solution : first->members()) {
      auto& individual = solution.individual();

      assign_nondominated_strength(solution, dominated_count);

      get_tag<frontier_level_t>(individual) = first->level();

      output.add_individual(individual);
    }

    output.next();

    for (auto&& frontier : std::ranges::subrange{std::ranges::next(first),
                                                 std::ranges::end(sorted)}) {
      for (auto&& solution : frontier.members()) {
        auto& individual = solution.individual();

        assign_dominated_strength(solution);

        get_tag<frontier_level_t>(individual) = frontier.level();

        output.add_individual(individual);
      }

      output.next();
    }

    output.finish();
    return output;
  }

  template<ranked_population<real_rank_t> Population, typename Pareto>
  auto operator()(Population& population, Pareto preserved) const {
    auto analyzed = details::prepare_strength_fast<real_rank_t>(population);

    auto dominated_count = std::ranges::count_if(
        analyzed, [](auto&& solution) { return !solution.nondominated(); });

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
    auto s = solution.dominated_total() / (dominated_count + 1.);
    get_tag<real_rank_t>(solution.individual()) = s;

    for (auto&& dominated : solution.dominated()) {
      get_tag<real_rank_t>(dominated.individual()) += s;
    }
  }

  template<typename Solution>
  inline static void assign_dominated_strength(Solution& solution) noexcept {
    get_tag<real_rank_t>(solution.individual()) += 1.;
  }
};

// number of dominated, all fronts (spea-ii)
class accumulated_strength {
public:
  template<ranked_population<int_rank_t> Population>
  auto operator()(Population& population, pareto_preserved_t /*unused*/) const {
    auto output = details::prepare_strength_slow<int_rank_t>(population);

    for (auto&& frontier :
         population.individuals() |
             pareto::views::sort(population.adopted_raw_comparator())) {
      for (auto&& solution : frontier.members()) {
        auto& individual = solution.individual();

        assign_strength(solution);

        get_tag<frontier_level_t>(individual) = frontier.level();

        output.add_individual(individual);
      }

      output.next();
    }

    output.finish();
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

} // namespace gal::rank
