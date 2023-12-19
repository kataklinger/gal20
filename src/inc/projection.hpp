
#pragma once

#include "multiobjective.hpp"

#include "statistics.hpp"

namespace gal {
namespace project {

  namespace details {

    template<typename Population, typename... From>
    concept projectable_from =
        std::constructible_from<get_fitness_t<scaled_fitness_tag, Population>,
                                From...>;

  } // namespace details

  template<typename Context, typename RankTag, typename... From>
  concept projectable_context =
      crowded_population<typename Context::population_t> &&
      ranked_population<typename Context::population_t, RankTag> &&
      details::projectable_from<typename Context::population_t, From...> &&
      std::convertible_to<RankTag, double>;

  // x = f(rank) / (1 - density) (nsga)
  template<typename RankTag, projectable_context<RankTag, double> Context>
  class scale {
  public:
    using context_t = Context;
    using population_t = typename context_t::population_t;

  private:
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit scale(context_t& context) noexcept
        : population_{&context.population()} {
    }

    template<typename Preserved>
    void operator()(population_pareto_t<population_t, Preserved>& sets,
                    cluster_set const& /*unused*/) const {
      std::vector<double> multipliers(sets.count(),
                                      std::numeric_limits<double>::max());

      for (auto&& individual : population_->individuals()) {
        auto front = get_tag<frontier_level_t>(individual).front();
        double density{get_tag<crowd_density_t>(individual)};

        if (multipliers[front] > density) {
          multipliers[front] = density;
        }
      }

      for (auto i = multipliers.size() - 1; i > 0; --i) {
        multipliers[i - 1] /= multipliers[i];
      }

      for (auto&& individual : population_->individuals()) {
        auto front = get_tag<frontier_level_t>(individual).front();
        auto scaled = multipliers[front] * get_tag<RankTag>(individual);

        individual.evaluation().set_scaled(scaled_fitness_t{scaled});
      }
    }

  private:
    population_t* population_;
  };

  // x = f(rank) + g(density) (spea-ii)
  template<typename RankTag, projectable_context<RankTag, double> Context>
  class translate {
  public:
    using context_t = Context;
    using population_t = typename context_t::population_t;

  private:
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit translate(context_t& context) noexcept
        : population_{&context.population()} {
    }

    template<typename Preserved>
    void operator()(population_pareto_t<population_t, Preserved>& /*unused*/,
                    cluster_set const& /*unused*/) const noexcept {
      for (auto&& individual : population_->individuals()) {
        double rank{get_tag<RankTag>(individual)};
        double density{get_tag<crowd_density_t>(individual)};

        individual.evaluation().set_scaled(scaled_fitness_t{rank + density});
      }
    }

  private:
    population_t* population_;
  };

  // x = <rank, density> (nsga-ii, spea)
  template<typename RankTag, projectable_context<RankTag, double> Context>
  class merge {
  public:
    using context_t = Context;
    using population_t = typename context_t::population_t;

  private:
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit merge(context_t& context) noexcept
        : population_{&context.population()} {
    }

    template<typename Preserved>
    void operator()(population_pareto_t<population_t, Preserved>& /*unused*/,
                    cluster_set const& /*unused*/) const noexcept {
      for (auto&& individual : population_->individuals()) {
        double rank{get_tag<RankTag>(individual)};
        double density{get_tag<crowd_density_t>(individual)};

        individual.evaluation().set_scaled(scaled_fitness_t{rank, density});
      }
    }

  private:
    population_t* population_;
  };

  template<typename Context, typename Tag>
  concept truncateable_context =
      population_tagged_with<typename Context::population_t,
                             Tag,
                             crowd_density_t> &&
      std::convertible_to<Tag, double> &&
      details::projectable_from<typename Context::population_t, double>;

  namespace details {

    template<typename Tag, typename Population>
    inline void assign_truncated(Population& population) noexcept {
      using scaled_fitness_t = get_fitness_t<scaled_fitness_tag, Population>;

      for (auto&& individual : population.individuals()) {
        double value{get_tag<Tag>(individual)};
        individual.evaluation().set_scaled(scaled_fitness_t{value});
      }
    }

  } // namespace details

  // x = rank or x = density (pesa, pesa-ii, paes)
  template<typename SelectedTag, truncateable_context<SelectedTag> Context>
  class truncate {
  public:
    using context_t = Context;
    using population_t = typename context_t::population_t;

  public:
    inline explicit truncate(context_t& context) noexcept
        : population_{&context.population()} {
    }

  public:
    template<typename Preserved>
    inline void
        operator()(population_pareto_t<population_t, Preserved>& /*unused*/,
                   cluster_set const& /*unused*/) const noexcept {
      details::assign_truncated<SelectedTag>(*population_);
    }

  private:
    population_t* population_;
  };

  template<typename Context, typename Tag>
  concept alternateable_context =
      truncateable_context<Context, Tag> &&
      stats::tracked_models<typename Context::statistics_t, stats::generation>;

  // x0 = rank, x1 = density, ... (rdga)
  template<typename RankTag, truncateable_context<RankTag> Context>
  class alternate {
  public:
    using context_t = Context;
    using population_t = typename context_t::population_t;

  public:
    inline explicit alternate(context_t& context) noexcept
        : context_{&context} {
    }

    template<typename Preserved>
    inline void
        operator()(population_pareto_t<population_t, Preserved>& /*unused*/,
                   cluster_set const& /*unused*/) const noexcept {
      if (auto& stats = context_->history().current();
          stats.generation_value() % 2 == 0) {
        details::assign_truncated<RankTag>(context_->population());
      }
      else {
        details::assign_truncated<crowd_density_t>(context_->population());
      }
    }

  private:
    context_t* context_;
  };

} // namespace project
} // namespace gal
