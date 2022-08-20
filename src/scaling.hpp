
#pragma once

#include "context.hpp"
#include "operation.hpp"

namespace gal {
namespace scale {

  template<auto Constant>
  concept scaling_constant = std::integral<decltype(Constant)> ||
      std::floating_point<decltype(Constant)>;

  // if( fmin > ( k * favg - fmax ) / ( k - 1 ) )
  //{
  //  d = fmax - favg;
  //
  //  if( fabs( d ) < 0.00001 )
  //   a = 1, b = 0;
  //  else
  //  {
  //   a = favg / d;
  //   b = a * ( fmax - k * favg );
  //   a *= ( k - 1 );
  //  }
  // }
  // else
  //{
  //  d = favg - fmin;
  //
  //  if( fabs( d ) < 0.00001 )
  //   a = 1, b = 0;
  //  else
  //  {
  //   a = favg / d;
  //   b = -fmin * a;
  //  }
  // }
  //  f` = a * f + b
  //  k factor, f fitness

  template<typename Context, auto Preassure>
  requires(scaling_constant<Preassure>) class linear {
  public:
    using is_global_t = std::true_type;
    using is_stable_t = std::true_type;
  };

  template<typename Context>
  class sigma {
  public:
    using is_global_t = std::true_type;

  private:
    using context_t = Context;

    using statistics_t = typename context_t::statistics_t;
    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using raw_fitness_t = typename population_t::raw_fitness_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    using fitness_avg_t = stat::average_fitness<raw_fitness_tag>;
    using fitness_dev_t = stat::fitness_deviation<raw_fitness_tag>;

  public:
    inline explicit sigma(context_t& context)
        : statistics_{&context.statistics()} {
    }

    inline void operator()(rank_t rank, individual_t& individual) const {
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
      return statistics_->template get<fitness_avg_t>().fitness_average_value();
    }

    inline auto& get_deviation() const noexcept {
      return statistics_->template get<fitness_dev_t>()
          .fitness_deviation_value();
    }

  private:
    statistics_t* statistics_;
  };

  template<typename Context, auto Preassure>
  requires(scaling_constant<Preassure>) class ranked {
  public:
    using is_stable_t = std::true_type;

  private:
    using context_t = Context;

    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit ranked(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(rank_t rank, individual_t& individual) const {
      auto eval = individual.evaluation();

      auto value = Preassure - 2.0 * (*rank - 1.0) * (Preassure - 1.0) /
                                   (population_->current_size() - 1.0);

      eval.set_scaled(scaled_fitness_t{value});
    }

  private:
    population_t* population_;
  };

  template<typename Context, auto Power>
  requires(scaling_constant<Power>) class power {
  public:
    using is_stable_t = std::true_type;

  private:
    using context_t = Context;

    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit power(context_t& context) {
    }

    inline void operator()(rank_t rank, individual_t& individual) {
      auto eval = individual.evaluation();
      eval.set_scaled(scaled_fitness_t{std::pow(eval.raw(), Power)});
    }
  };

  template<typename Context, auto Base>
  requires(scaling_constant<Base>) class exponential {
  public:
    using is_stable_t = std::true_type;

  private:
    using context_t = Context;

    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit exponential(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(rank_t rank, individual_t& individual) const {
      auto eval = individual.evaluation();

      auto power = population_->current_size() - *rank - 1;
      eval.set_scaled(scaled_fitness_t{std::pow(Base, power)});
    }

  private:
    population_t* population_;
  };

  template<typename Context, auto RankCutoff, auto Proportion>
  requires(std::integral<decltype(RankCutoff)>&& RankCutoff > 0 &&
           scaling_constant<Proportion>) class top {
  public:
    using is_stable_t = std::true_type;

  private:
    using context_t = Context;

    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    inline static constexpr std::size_t cutoff{RankCutoff};

  public:
    inline explicit top(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(rank_t rank, individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(
          scaled_fitness_t{*rank <= cutoff ? Proportion * eval.raw() : 0});
    }

  private:
    population_t* population_;
  };

  template<typename Context>
  class window {
  public:
    using is_global_t = std::true_type;
    using is_stable_t = std::true_type;

  private:
    using context_t = Context;

    using statistics_t = typename context_t::statistics_t;
    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

    using fitness_min_t = stat::extreme_fitness<raw_fitness_tag>;

  public:
    inline explicit window(context_t& context)
        : statistics_{&context.statistics()} {
    }

    inline void operator()(rank_t rank, individual_t& individual) const {
      auto eval = individual.evaluation();
      eval.set_scaled(scaled_fitness_t{eval.raw() - get_worst()});
    }

  private:
    inline auto& get_worst() const noexcept {
      return statistics_->template get<fitness_min_t>().fitness_worst_value();
    }

  private:
    statistics_t* statistics_;
  };

  template<template<typename, auto...> class Scaling, auto... Parameters>
  requires(scaling_constant<Parameters>&&...) struct factory {
    template<typename Context>
    auto operator()(Context& context) {
      return Scaling<Context, Parameters...>{context};
    }
  };

} // namespace scale
} // namespace gal
