
#pragma once

#include "multiobjective.hpp"

#include "statistics.hpp"

namespace gal::project {

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
    std::convertible_to<typename tag_adopted_traits<RankTag>::value_t, double>;
// x = empty (pesa-ii)
template<typename Context>
class none {
public:
  using context_t = Context;
  using population_t = typename context_t::population_t;

public:
  inline explicit none(context_t& /*unused*/) noexcept {
  }

  template<typename Preserved>
  void operator()(population_pareto_t<population_t, Preserved>& /*unused*/,
                  cluster_set const& /*unused*/) const {
  }
};

// x = f(rank) / (1 - density) (nsga)
template<typename Context, typename RankTag>
  requires(projectable_context<Context, RankTag, double>)
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
    std::vector<double> multipliers(sets.size(),
                                    std::numeric_limits<double>::max());

    for (auto&& individual : population_->individuals()) {
      auto front = get_tag<frontier_level_t>(individual).get() - 1;
      double density{get_tag<crowd_density_t>(individual)};

      if (multipliers[front] > density) {
        multipliers[front] = density;
      }
    }

    for (auto correction = 1.0;
         auto&& multiplier : multipliers | std::views::reverse) {
      correction *= std::exchange(multiplier, multiplier / correction);
    }

    for (auto&& individual : population_->individuals()) {
      auto front = get_tag<frontier_level_t>(individual).get() - 1;
      auto scaled = multipliers[front] * get_tag<RankTag>(individual).get() *
                    get_tag<crowd_density_t>(individual).get();

      individual.evaluation().set_scaled(scaled_fitness_t{scaled});
    }
  }

private:
  population_t* population_;
};

// x = f(rank) + g(density) (spea-ii)
template<typename Context, typename RankTag>
  requires(projectable_context<Context, RankTag, double>)
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
      auto rank = get_tag<RankTag>(individual).get();
      auto density = get_tag<crowd_density_t>(individual).get();

      individual.evaluation().set_scaled(scaled_fitness_t{rank + density});
    }
  }

private:
  population_t* population_;
};

// x = <rank, density> (nsga-ii)
template<typename Context, typename RankTag>
  requires(projectable_context<Context,
                               RankTag,
                               typename tag_adopted_traits<RankTag>::value_t,
                               double>)
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
      auto rank = get_tag<RankTag>(individual).get();
      auto density = get_tag<crowd_density_t>(individual).get();

      individual.evaluation().set_scaled(scaled_fitness_t{rank, density});
    }
  }

private:
  population_t* population_;
};

template<typename Context, typename Tag>
concept truncateable_context =
    population_tagged_with<typename Context::population_t, Tag> &&
    std::convertible_to<typename tag_adopted_traits<Tag>::value_t, double> &&
    details::projectable_from<typename Context::population_t, double>;

namespace details {

  template<typename Tag, typename Population>
  inline void assign_truncated(Population& population) noexcept {
    using scaled_fitness_t = get_fitness_t<scaled_fitness_tag, Population>;

    for (auto&& individual : population.individuals()) {
      auto value = static_cast<double>(get_tag<Tag>(individual).get());
      individual.evaluation().set_scaled(scaled_fitness_t{value});
    }
  }

} // namespace details

// x = rank or x = density (spea, pesa, paes)
template<typename Context, typename SelectedTag>
  requires(truncateable_context<Context, SelectedTag>)
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
    population_tagged_with<typename Context::population_t, crowd_density_t> &&
    stats::tracked_models<typename Context::statistics_t, stats::generation>;

// x0 = rank, x1 = density, ... (rdga)
template<typename Context, typename RankTag>
  requires(truncateable_context<Context, RankTag>)
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

template<template<typename...> class Projection, typename... Tys>
class factory {
public:
  template<typename Context>
  inline constexpr auto operator()(Context& context) const {
    return Projection<Context, Tys...>{context};
  }
};

} // namespace gal::project
