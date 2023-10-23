
#pragma once

#include "pareto.hpp"
#include "population.hpp"

namespace gal {
namespace rank {

  template<typename Value>
  concept rank_value = std::regular<Value> && std::totally_ordered<Value>;

  template<rank_value Value>
  class rank_tag {
  public:
    using value_t = Value;

  public:
    inline rank_tag(value_t value) noexcept
        : value_{value} {
    }

    inline operator value_t() const noexcept {
      return value_;
    }

    inline rank_tag& operator=(value_t value) noexcept {
      value_ = value;
      return *this;
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

  using frontier_id = std::size_t;

  using bin_rank_t = rank_tag<binary_rank>;
  using int_rank_t = rank_tag<std::size_t>;
  using real_rank_t = rank_tag<double>;

  template<typename Population, typename RankTag>
  concept ranked_population =
      multiobjective_population<Population> && tagged_with<Population, RankTag>;

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
    inline void clean(Population& population) {
      for (auto&& individual : population.individuals()) {
        get<RankTag>(individual) = {};
      }
    }

  } // namespace details

  struct fronts_preserved_t {};
  inline constexpr fronts_preserved_t fronts_preserved{};

  struct fronts_reduced_t {};
  inline constexpr fronts_reduced_t fronts_reduced{};

  struct fronts_nondominated_t {};
  inline constexpr fronts_nondominated_t fronts_nondominated{};

  struct fronts_erased_t {};
  inline constexpr fronts_erased_t fronts_erased{};

  template<typename Individual>
  class pareto_fronts {
  public:
    using individual_t = Individual;
    using individuals_t = std::vector<individual_t*>;
    using front_boundery = typename std::vector<individual_t*>::iterator;

  public:
    inline explicit pareto_fronts(std::size_t max_individuals)
        : max_individuals_{max_individuals} {
      individuals_.reserve(max_individuals);
    }

    inline void add_inidividual(individual_t& individual) {
      assert(individuals_.size() < max_individuals_);
      individuals_.push_back(&individual);
    }

    inline void next() {
      front_boundaries_.push_back(individuals_.end());
    }

    inline auto count() const noexcept {
      front_boundaries_.size() + 1;
    }

    inline auto at(std::size_t front) const noexcept {
      return std::ranges::subrange{front == 0 ? individuals_.begin()
                                              : front_boundaries_[front - 1],
                                   front_boundaries_[front]};
    }

    inline auto operator[](std::size_t front) const noexcept {
      return at(front);
    }

  private:
    std::size_t max_individuals_;
    std::vector<individual_t*> individuals_;
    std::vector<front_boundery> front_boundaries_;
  };

  namespace details {

    template<typename Population>
    using population_fronts = pareto_fronts<typename Population::individual_t>;

    template<typename Population, typename Preserve>
    class wrapped_fronts;

    template<typename Population>
    class wrapped_fronts<Population, fronts_preserved_t> {
    public:
      using base_fronts_t = population_fronts<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_fronts(std::size_t max_individuals)
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
      base_fronts_t base_;
    };

    template<typename Population>
    class wrapped_fronts<Population, fronts_reduced_t> {
    public:
      using base_fronts_t = population_fronts<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_fronts(std::size_t max_individuals)
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
      base_fronts_t base_;
    };

    template<typename Population>
    class wrapped_fronts<Population, fronts_nondominated_t> {
    public:
      using base_fronts_t = population_fronts<Population>;
      using individual_t = typename Population::individual_t;

    public:
      inline explicit wrapped_fronts(std::size_t max_individuals)
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
      base_fronts_t base_;
    };

    template<typename Population>
    class wrapped_fronts<Population, fronts_erased_t> {
      using individual_t = typename Population::individual_t;

      inline explicit wrapped_fronts(std::size_t /*unused*/) {
      }

      inline void add_inidividual(individual_t& individual) noexcept {
      }

      inline void next() noexcept {
      }

      inline auto unwrap() noexcept {
        return population_fronts<Population>{};
      }
    };

    template<typename Population>
    inline void
        populate_binary_front(Population& population,
                              details::population_fronts<Population>& fronts,
                              binary_rank which) {
      for (auto&& individual : population.individuals()) {
        if (details::get<bin_rank_t>(individual) == which) {
          fronts.add_individuals(individual);
        }
      }

      fronts.next();
    }

    template<typename Population>
    inline auto generate_binary_fronts(Population& population,
                                       fronts_reduced_t /*unused*/) {
      details::population_fronts<Population> fronts{population.size()};

      populate_binary_front(population, fronts, binary_rank::nondominated);
      populate_binary_front(population, fronts, binary_rank::dominated);

      return fronts;
    }

    template<typename Population>
    inline auto generate_binary_fronts(Population& population,
                                       fronts_nondominated_t /*unused*/) {
      details::population_fronts<Population> fronts{population.size()};

      populate_binary_front(population, fronts, binary_rank::nondominated);

      return fronts;
    }

    template<typename Population>
    inline auto generate_binary_fronts(Population& population,
                                       fronts_erased_t /*unused*/) {
      return details::population_fronts<Population>{};
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
                    fronts_preserved_t /*unused*/) const {
      details::clean<bin_rank_t>(population);

      details::population_fronts<Population> fronts{population.size()};
      auto current = binary_rank::nondominated;

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& ind = solution.individual();
          details::get<bin_rank_t>(ind) = current;

          fronts.add_inidividual(ind);
        }

        current = binary_rank::dominated;
        fronts.next();
      }

      return fronts;
    }

    template<ranked_population<bin_rank_t> Population, typename Fronts>
    void operator()(Population& population, Fronts preserve) const {
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

      return details::generate_binary_fronts(population, preserve);
    }
  };

  // pareto front level (nsga & nsga-ii)
  class level {
  public:
    template<ranked_population<int_rank_t> Population, typename Fronts>
    auto operator()(Population& population, Fronts /*unused*/) const {
      details::clean<int_rank_t>(population);

      details::wrapped_fronts<Population, Fronts> fronts{population.size()};

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          auto& ind = solution.individual();
          details::get<int_rank_t>(ind) = frontier.level();

          fronts.add_individual(ind);
        }

        fronts.next();
      }

      return fronts.extract();
    }
  };

  namespace details {

    template<typename Population, typename RankTag>
    inline auto prepare_strength_fast(Population& pop) {
      clean<RankTag>(pop);
      return pareto::analyze(pop.indviduals(), pop.raw_comparator());
    }

    template<typename Population, typename RankTag>
    inline auto prepare_strength_slow(Population& pop) {
      clean<RankTag>(pop);
      return population_fronts<Population>{pop.size()};
    }

    template<typename Solutions, typename Front>
    inline void populate_strength_front(Solutions& solutions,
                                        Front& fronts,
                                        bool which) {
      for (auto&& solution : solutions) {
        if (solution.nondominated() == which) {
          fronts.add_individuals(solution.individual());
        }
      }

      fronts.next();
    }

    template<typename Solutions>
    inline auto generate_strength_fronts(Solutions& solutions,
                                         std::size_t size,
                                         fronts_reduced_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      pareto_fronts<individual_t> fronts{size};

      populate_strength_front(solutions, fronts, true);
      populate_strength_front(solutions, fronts, false);

      return fronts;
    }

    template<typename Solutions>
    inline auto generate_strength_fronts(Solutions& solutions,
                                         std::size_t size,
                                         fronts_nondominated_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      pareto_fronts<individual_t> fronts{size};

      populate_strength_front(solutions, fronts, true);

      return fronts;
    }

    template<typename Solutions>
    inline auto generate_strength_fronts(Solutions& solutions,
                                         std::size_t /*unused*/,
                                         fronts_erased_t /*unused*/) {
      using individual_t = typename Solutions::value_type::individual_t;
      return pareto_fronts<individual_t>{};
    }

  } // namespace details

  // accumulated pareto level (rdga)
  class accumulated_level {
  public:
    template<ranked_population<int_rank_t> Population, typename Fronts>
    void operator()(Population& population, Fronts /*unused*/) const {
      details::clean<int_rank_t>(population);

      details::wrapped_fronts<Population, Fronts> fronts{population.size()};

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

          fronts.add_individual(ind);
        }

        fronts.next();
      }

      return fronts.extract();
    }
  };

  // number of dominated, first front (spea)
  class strength {
  public:
    template<ranked_population<real_rank_t> Population>
    auto operator()(Population& population,
                    fronts_preserved_t /*unused*/) const {
      auto fronts = details::prepare_strength_slow(population);

      auto sorted = population.indviduals() |
                    pareto::views::sort(population.raw_comparator());

      auto first = std::ranges::begin(sorted);

      auto dominated_count =
          population.size() - std::ranges::size(first->members());

      for (auto&& solution : first->members()) {
        assign_nondominated_strength(solution, dominated_count);

        fronts.add_inidividual(solution.individual());
      }

      fronts.next();

      for (auto&& frontier : std::ranges::subrange{std::ranges::next(first),
                                                   std::ranges::end(sorted)}) {
        for (auto&& solution : frontier.members()) {
          assign_dominated_strength(solution);

          fronts.add_inidividual(solution.individual());
        }

        fronts.next();
      }

      return fronts;
    }

    template<ranked_population<real_rank_t> Population, typename Fronts>
    auto operator()(Population& population, Fronts preserved) const {
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

      return details::generate_strength_fronts(
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
                    fronts_preserved_t /*unused*/) const {
      auto fronts = details::prepare_strength_slow(population);

      for (auto&& frontier :
           population.indviduals() |
               pareto::views::sort(population.raw_comparator())) {
        for (auto&& solution : frontier.members()) {
          assign_strength(solution);

          fronts.add_inidividual(solution.individual());
        }

        fronts.next();
      }

      return fronts;
    }

    template<ranked_population<int_rank_t> Population, typename Fronts>
    auto operator()(Population& population, Fronts preserved) const {
      auto analyzed = details::prepare_strength_fast<int_rank_t>(population);

      for (auto&& solution : analyzed) {
        assign_strength(solution);
      }

      return details::generate_strength_fronts(
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
