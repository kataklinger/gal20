
#pragma once

#include "context.hpp"
#include "operation.hpp"

namespace gal {
namespace scale {

  template<auto Constant>
  concept scaling_constant = util::arithmetic<decltype(Constant)>;

  namespace details {

    using linear_coefficients = std::pair<double, double>;

    inline bool approaching_zero(double delta) noexcept {
      return std::fabs(delta) < 0.00001;
    }

    template<auto Preassure, fitness Fitness>
    linear_coefficients caclulate_linear_coefficients(Fitness const& fmin,
                                                      Fitness const& favg,
                                                      Fitness const& fmax) {
      if (fmin > (Preassure * favg - fmax) / (Preassure - 1.0)) {
        auto delta = fmax - favg;
        if (approaching_zero(delta)) {
          return {1, 0};
        }

        auto a = favg / delta;
        return {a * (Preassure - 1.0), a * (fmax - Preassure * favg)};
      }
      else {
        auto delta = favg - fmin;
        if (approaching_zero(delta)) {
          return {1, 0};
        }

        auto a = favg / delta;
        return {a, -fmin * a};
      }
    }

  } // namespace details

  template<typename Fitness>
  concept linear_fitness = averageable_fitness<Fitness> && requires(Fitness f) {
    { 1.0 * f } -> std::convertible_to<double>;
  };

  template<typename Context>
  concept linear_context =
      linear_fitness<typename Context::population_t::raw_fitness_t> &&
      std::constructible_from<typename Context::population_t::scaled_fitness_t,
                              double> &&
      stats::tracked_models<typename Context::statistics_t,
                            stats::extreme_fitness<raw_fitness_tag>,
                            stats::average_fitness<raw_fitness_tag>>;

  template<linear_context Context, auto Preassure>
    requires(scaling_constant<Preassure>)
  class linear {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using statistics_t = typename context_t::statistics_t;
    using population_t = typename context_t::population_t;
    using raw_fitness_t = typename population_t::raw_fitness_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    using fitness_ext_t = stats::extreme_fitness<raw_fitness_tag>;
    using fitness_avg_t = stats::average_fitness<raw_fitness_tag>;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit linear(context_t& context)
        : statistics_{&context.history()} {
    }

    inline void operator()() noexcept {
      coefficients_ = details::caclulate_linear_coefficients<Preassure>(
          get_worst(), get_average(), get_best());
    }

    inline void operator()(ordinal_t /*unused*/,
                           individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(calculate(eval.raw()));
    }

  private:
    inline scaled_fitness_t calculate(raw_fitness_t const& raw) const {
      return {coefficients_.first * raw + coefficients_.second};
    }

    inline auto& get_average() const noexcept {
      return statistics_->current()
          .template get<fitness_avg_t>()
          .fitness_average_value();
    }

    inline auto& get_best() const noexcept {
      return statistics_->current()
          .template get<fitness_ext_t>()
          .fitness_best_value();
    }

    inline auto& get_worst() const noexcept {
      return statistics_->current()
          .template get<fitness_ext_t>()
          .fitness_worst_value();
    }

  private:
    std::pair<double, double> coefficients_;
    stats::history<statistics_t>* statistics_;
  };

  template<typename Fitness>
  concept sigma_fitness =
      stats::deviation_fitness<Fitness> &&
      std::convertible_to<stats::fitness_deviation_t<Fitness>, double> &&
      requires(Fitness f) {
        { f / 1.0 } -> std::convertible_to<double>;
      };

  template<typename Context>
  concept sigma_context =
      sigma_fitness<typename Context::population_t::raw_fitness_t> &&
      std::constructible_from<typename Context::population_t::scaled_fitness_t,
                              double> &&
      stats::tracked_models<typename Context::statistics_t,
                            stats::fitness_deviation<raw_fitness_tag>>;

  template<sigma_context Context>
  class sigma {
  public:
    using context_t = Context;

  private:
    using statistics_t = typename context_t::statistics_t;
    using population_t = typename context_t::population_t;
    using raw_fitness_t = typename population_t::raw_fitness_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    using fitness_avg_t = stats::average_fitness<raw_fitness_tag>;
    using fitness_dev_t = stats::fitness_deviation<raw_fitness_tag>;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit sigma(context_t& context)
        : statistics_{&context.history()} {
    }

    inline void operator()(ordinal_t /*unused*/,
                           individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(calculate(eval.raw()));
    }

  private:
    inline scaled_fitness_t calculate(raw_fitness_t const& raw) const noexcept {
      if (auto dev = get_deviation(); dev > 0) {
        return {1.0 + (raw - get_average()) / (2.0 * dev)};
      }
      else {
        return {1.0};
      }
    }

    inline auto& get_average() const noexcept {
      return statistics_->current()
          .template get<fitness_avg_t>()
          .fitness_average_value();
    }

    inline auto& get_deviation() const noexcept {
      return statistics_->current()
          .template get<fitness_dev_t>()
          .fitness_deviation_value();
    }

  private:
    stats::history<statistics_t>* statistics_;
  };

  template<typename Context>
  concept ranked_context =
      ordered_population<typename Context::population_t, raw_fitness_tag> &&
      std::constructible_from<typename Context::population_t::scaled_fitness_t,
                              double>;

  template<ranked_context Context, auto Preassure>
    requires(scaling_constant<Preassure>)
  class ranked {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using population_t = typename context_t::population_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit ranked(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(ordinal_t ordinal, individual_t& individual) const {
      auto eval = individual.evaluation();

      auto value = Preassure - 2.0 * (*ordinal - 1.0) * (Preassure - 1.0) /
                                   (population_->current_size() - 1.0);

      eval.set_scaled(scaled_fitness_t{value});
    }

  private:
    population_t* population_;
  };

  template<typename Context, typename Base>
  concept exponential_context =
      ordered_population<typename Context::population_t, raw_fitness_tag> &&
      std::constructible_from<typename Context::population_t::scaled_fitness_t,
                              Base>;

  template<typename Context, auto Base>
    requires(exponential_context<Context, decltype(Base)> &&
             scaling_constant<Base>)
  class exponential {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using population_t = typename context_t::population_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit exponential(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(ordinal_t ordinal, individual_t& individual) const {
      auto eval = individual.evaluation();

      auto power = population_->current_size() - *ordinal - 1;
      eval.set_scaled(scaled_fitness_t{std::pow(Base, power)});
    }

  private:
    population_t* population_;
  };

  template<typename Fitness, typename Proportion, typename Output>
  concept proportional_fitness = requires(Fitness f, Proportion p) {
    { p* f } -> std::convertible_to<Output>;
  };

  template<typename Context, typename Proportion>
  concept proportional_context =
      ordered_population<typename Context::population_t, raw_fitness_tag> &&
      proportional_fitness<typename Context::population_t::raw_fitness_t,
                           Proportion,
                           typename Context::population_t::scaled_fitness_t>;

  template<typename Context, auto OrdinalCutoff, auto Proportion>
    requires(proportional_context<Context, decltype(Proportion)> &&
             std::integral<decltype(OrdinalCutoff)> && OrdinalCutoff > 0 &&
             scaling_constant<Proportion>)
  class top {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using population_t = typename context_t::population_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    using individual_t = typename population_t::individual_t;

    inline static constexpr std::size_t cutoff{OrdinalCutoff};

  public:
    inline explicit top(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(ordinal_t ordinal, individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(
          scaled_fitness_t{*ordinal <= cutoff ? Proportion * eval.raw() : 0});
    }

  private:
    population_t* population_;
  };

  template<typename Fitness, typename Power, typename Output>
  concept power_fitness = requires(Fitness f, Power p) {
    { std::pow(f, p) } -> std::convertible_to<Output>;
  };

  template<typename Context, typename Power>
  concept power_context =
      power_fitness<typename Context::population_t::raw_fitness_t,
                    Power,
                    typename Context::population_t::scaled_fitness_t>;

  template<typename Context, auto Power>
    requires(power_context<Context, decltype(Power)> && scaling_constant<Power>)
  class power {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using population_t = typename context_t::population_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit power(context_t& /*unused*/) {
    }

    inline void operator()(individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(scaled_fitness_t{std::pow(eval.raw(), Power)});
    }
  };

  template<typename Context>
  concept window_context =
      averageable_fitness<typename Context::population_t::raw_fitness_t> &&
      std::constructible_from<typename Context::population_t::scaled_fitness_t,
                              typename Context::population_t::raw_fitness_t> &&
      stats::tracked_models<typename Context::statistics_t,
                            stats::extreme_fitness<raw_fitness_tag>>;

  template<window_context Context>
  class window {
  public:
    using is_stable_t = std::true_type;

    using context_t = Context;

  private:
    using statistics_t = typename context_t::statistics_t;
    using population_t = typename context_t::population_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    using fitness_ext_t = stats::extreme_fitness<raw_fitness_tag>;

  public:
    using individual_t = typename population_t::individual_t;

  public:
    inline explicit window(context_t& context)
        : statistics_{&context.history()} {
    }

    inline void operator()(ordinal_t /*unused*/,
                           individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(scaled_fitness_t{eval.raw() - get_worst()});
    }

  private:
    inline auto& get_worst() const noexcept {
      return statistics_->current()
          .template get<fitness_ext_t>()
          .fitness_worst_value();
    }

  private:
    stats::history<statistics_t>* statistics_;
  };

  template<template<typename, auto...> class Scaling, auto... Parameters>
    requires(scaling_constant<Parameters> && ...)
  struct factory {
    template<typename Context>
    auto operator()(Context& context) {
      return Scaling<Context, Parameters...>{context};
    }
  };

} // namespace scale
} // namespace gal
