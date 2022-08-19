
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
  //  	a = 1, b = 0;
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
  class linear {};

  // f` = 1 + (f - favg) / 2 * d, d != 0; 1, d = 0
  // f fitness, d std. deviation
  class sigma {};

  // f` = (p - 2) * (r - 1)(p - 1) / (N - 1)
  // p preasure, r rank, N population size
  class ranked {};

  template<typename Context, auto Power>
  requires(scaling_constant<Power>) class power {
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

    inline void operator()(rank_t rank, individual_t& individual) {
      auto eval = individual.evaluation();

      auto power = population_->current_size() - *rank - 1;
      eval.set_scaled(scaled_fitness_t{std::pow(Base, power)});
    }

  private:
    population_t* population_;
  };

  template<typename Context, std::size_t RankCutoff, auto Proportion>
  requires(scaling_constant<Proportion>) class top {
  private:
    using context_t = Context;

    using population_t = typename context_t::population_t;
    using individual_t = typename population_t::individual_t;
    using scaled_fitness_t = typename population_t::scaled_fitness_t;

  public:
    inline explicit top(context_t& context)
        : population_{&context.population()} {
    }

    inline void operator()() {
      population_->sort(raw_fitness_tag{});
    }

    inline void operator()(rank_t rank, individual_t& individual) {
      auto eval = individual.evaluation();
      eval.set_scaled(
          scaled_fitness_t{*rank <= RankCutoff ? Proportion * eval.raw() : 0});
    }

  private:
    population_t* population_;
  };

  template<typename Context>
  class window {
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

    inline void operator()(rank_t rank, individual_t& individual) {
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

} // namespace scale
} // namespace gal
