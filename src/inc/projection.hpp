
#pragma once

#include "multiobjective.hpp"
#include "statistics.hpp"

#include <utility>

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
class none {
public:
  template<typename Context, typename Preserved>
  void operator()(Context& /*unused*/,
                  population_pareto_t<typename Context::population_t,
                                      Preserved>& /*unused*/,
                  cluster_set const& /*unused*/) const {
  }
};

// x = rank / (1. - density) (nsga)
template<typename RankTag>
class scale {
public:
  template<projectable_context<RankTag, double> Context, typename Preserved>
  void operator()(
      Context& context,
      population_pareto_t<typename Context::population_t, Preserved>& sets,
      cluster_set const& /*unused*/) const {
    using scaled_fitness_t = typename Context::population_t::scaled_fitness_t;

    std::vector<double> multipliers(sets.size(),
                                    std::numeric_limits<double>::max());

    for (auto&& individual : context.population().individuals()) {
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

    for (auto&& individual : context.population().individuals()) {
      auto front = get_tag<frontier_level_t>(individual).get() - 1;
      auto scaled = multipliers[front] * get_tag<RankTag>(individual).get() *
                    get_tag<crowd_density_t>(individual).get();

      individual.eval().set_scaled(scaled_fitness_t{scaled});
    }
  }
};

// x = rank + (1. - density) (spea-ii)
template<typename RankTag>
class translate {
public:
  template<projectable_context<RankTag, double> Context, typename Preserved>
  void operator()(Context& context,
                  population_pareto_t<typename Context::population_t,
                                      Preserved>& /*unused*/,
                  cluster_set const& /*unused*/) const noexcept {
    using scaled_fitness_t = typename Context::population_t::scaled_fitness_t;

    for (auto&& individual : context.population().individuals()) {
      auto rank = get_tag<RankTag>(individual).get();
      auto density = 1. - get_tag<crowd_density_t>(individual).get();

      individual.eval().set_scaled(scaled_fitness_t{rank + density});
    }
  }
};

// x = <rank, density> (nsga-ii)
template<typename RankTag>
class merge {
public:
  template<projectable_context<RankTag,
                               typename tag_adopted_traits<RankTag>::value_t,
                               double> Context,
           typename Preserved>
  void operator()(Context& context,
                  population_pareto_t<typename Context::population_t,
                                      Preserved>& /*unused*/,
                  cluster_set const& /*unused*/) const noexcept {
    using scaled_fitness_t = typename Context::population_t::scaled_fitness_t;

    for (auto&& individual : context.population().individuals()) {
      auto rank = get_tag<RankTag>(individual).get();
      auto density = 1. - get_tag<crowd_density_t>(individual).get();

      individual.eval().set_scaled(scaled_fitness_t{rank, density});
    }
  }
};

template<typename Context, typename Tag>
concept truncateable_context =
    population_tagged_with<typename Context::population_t, Tag> &&
    std::convertible_to<typename tag_adopted_traits<Tag>::value_t, double> &&
    details::projectable_from<typename Context::population_t, double>;

namespace details {

  template<typename Tag, typename Population>
  inline void assign_truncated(Population& population) noexcept {
    using scaled_fitness_t = typename Population::scaled_fitness_t;

    for (auto&& individual : population.individuals()) {
      auto value = static_cast<double>(get_tag<Tag>(individual).get());

      if constexpr (std::is_same_v<Tag, crowd_density_t>) {
        value = 1. - value;
      }

      individual.eval().set_scaled(scaled_fitness_t{value});
    }
  }

} // namespace details

// x = rank or x = 1. - density (pesa, paes)
template<typename SelectedTag>
class truncate {
public:
  template<truncateable_context<SelectedTag> Context, typename Preserved>
  inline void operator()(Context& context,
                         population_pareto_t<typename Context::population_t,
                                             Preserved>& /*unused*/,
                         cluster_set const& /*unused*/) const noexcept {
    details::assign_truncated<SelectedTag>(context.population());
  }
};

template<typename Context, typename Tag>
concept alternateable_context =
    truncateable_context<Context, Tag> &&
    population_tagged_with<typename Context::population_t, crowd_density_t> &&
    stats::tracked_models<typename Context::statistics_t, stats::generation>;

// x0 = rank, x1 = 1. - density, ... (rdga)
template<typename RankTag>
class alternate {
public:
  template<truncateable_context<RankTag> Context, typename Preserved>
  inline void operator()(Context& context,
                         population_pareto_t<typename Context::population_t,
                                             Preserved>& /*unused*/,
                         cluster_set const& /*unused*/) const noexcept {
    if (auto& stats = context.history().current();
        stats.generation_value() % 2 == 0) {
      details::assign_truncated<RankTag>(context.population());
    }
    else {
      details::assign_truncated<crowd_density_t>(context.population());
    }
  }
};

// x = f(chrosome) (spea)
template<typename Fn>
class custom {
public:
  inline explicit custom(Fn const& fn)
      : fn_{fn} {
  }

public:
  template<typename Context, typename Preserved>
  void operator()(Context& context,
                  population_pareto_t<typename Context::population_t,
                                      Preserved>& /*unused*/,
                  cluster_set const& /*unused*/) const noexcept {
    using scaled_fitness_t = typename Context::population_t::scaled_fitness_t;

    for (auto&& individual : context.population().individuals()) {
      auto value = std::invoke(fn_, individual);

      individual.eval().set_scaled(scaled_fitness_t{value});
    }
  }

private:
  Fn fn_;
};

} // namespace gal::project
