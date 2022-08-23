
#pragma once

#include "operation.hpp"

namespace gal {
namespace couple {

  template<auto Probability>
  concept probability = Probability >= 0.f && Probability <= 1.f &&
                        std::floating_point<decltype(Probability)>;

  template<typename Generator, auto Probability>
  requires(probability<Probability>) struct probabilistic_operation {
  public:
    using generator_t = Generator;
    using distribution_t =
        std::uniform_real_distribution<decltype(Probability)>;

  public:
    inline explicit probabilistic_operation(generator_t& generator) noexcept
        : generator_{&generator_}
        , distribution_{0.f, 1.f} {
    }

    inline bool operator()() const {
      if constexpr (Probability == 0.f) {
        return false;
      }
      else if constexpr (Probability == 1.f) {
        return true;
      }

      return dist(*generator_) < Probability;
    }

  private:
    generator_t* generator_;
    distribution_t distribution_;
  };

  template<typename Generator,
           auto Crossover,
           auto Mutation,
           traits::boolean_flag MutationImproveOnly>
  requires(probability<Crossover>&&
               probability<Mutation>) class reproduction_params {
  public:
    using generator_t = Generator;
    using mutation_improve_only_t = MutationImproveOnly;

    inline static constexpr auto crossover_probability = Crossover;
    inline static constexpr auto mutation_probability = Mutation;

  public:
    inline explicit reproduction_params(generator_t& generator) noexcept
        : crossover_{generator}
        , mutation_{generator} {
    }

    inline auto const& crossover() const noexcept {
      return crossover_;
    }

    inline auto const& mutation() const noexcept {
      return mutation_;
    }

  private:
    probabilistic_operation<generator_t, Crossover> crossover_;
    probabilistic_operation<generator_t, Mutation> mutation_;
  };

  namespace details {

    template<typename Context, typename Population>
    concept with_scaling = requires(Context ctx) {
      { ctx.scaling() } -> scaling<Population>;
    };

    template<typename Context, typename Population>
    concept without_scaling = !with_scaling<Context, Population>;

    template<typename Ty, traits::boolean_flag Condition>
    inline decltype(auto) move_if(Ty&& value, Condition /*unused*/) noexcept {
      if constexpr (Condition::value) {
        return std::move(value);
      }
      else {
        return std::forward<Ty>(value);
      }
    }

    template<typename Mutation, typename Chromosome, typename Probability>
    Chromosome mutate(Mutation const& mutation,
                      Chromosome&& chromosome,
                      Probability const& probability) {
      if (std::invoke(probability)) {
        if constexpr (std::is_lvalue_reference_v<Chromosome>) {
          Chromosome mutated{chromosome};
          std::invoke(mutation, mutated);
          return std::move(mutated);
        }
        else {
          std::invoke(mutation, chromosome);
          return std::move(chromosome);
        }
      }
      else {
        return std::move(chromosome);
      }
    }

    template<typename Individual,
             typename Context,
             typename Chromosome,
             typename Params>
    Individual try_mutate(Context const& context,
                          Chromosome& original,
                          Params const& params) {

      using improve_t = typename Params::mutation_improve_only_t;

      auto mutated = mutate(context.mutation(),
                            move_if(original, std::negation<improve_t>{}),
                            params.mutation());

      auto mutated_fitness = std::invoke(context.evaluator(), mutated);

      if constexpr (improve_t::value) {
        auto original_fitness = std::invoke(context.evaluator(), original);

        if (original_fitness > mutated_fitness) {
          return {std::move(original), {std::move(original_fitness)}};
        }
      }

      return {std::move(mutated), {std::move(mutated_fitness)}};
    }

    template<typename Crossover, typename Chromosome, typename Probability>
    inline std::pair<Chromosome, Chromosome>
        try_crossover(Crossover const& crossover,
                      Chromosome const& parent1,
                      Chromosome const& parent2,
                      Probability const& probability) {
      if (std::invoke(probability)) {
        return std::invoke(crossover, parent1, parent2);
      }
      else {
        return {parent1, parent2};
      }
    }

    template<typename Population,
             typename Context,
             typename Chromosome,
             typename Params>
    auto reproduce_impl(Context const& context,
                        Chromosome const& parent1,
                        Chromosome const& parent2,
                        Params const& params) {
      using individual_t = typename Population::individual_t;

      auto offspring = try_crossover(
          context.crossover(), parent1, parent2, params.crossover());

      return std::pair<individual_t, individual_t>{
          try_mutate<individual_t>(context, offspring.first, params),
          try_mutate<individual_t>(context, offspring.second, params)};
    }

    template<typename Population,
             without_scaling<Population> Context,
             typename Chromosome,
             typename Params>
    inline auto reproduce(Context const& context,
                          Chromosome const& parent1,
                          Chromosome const& parent2,
                          Params const& params) {
      return;
    }

    template<typename Population,
             with_scaling<Population> Context,
             typename Chromosome,
             typename Params>
    auto reproduce(Context const& context,
                   Chromosome const& parent1,
                   Chromosome const& parent2,
                   Params const& params) {
      auto offspring = reproduce_impl(context, parent1, parent2, params);
      std::invoke(context.scaling(), offspring.first);
      std::invoke(context.scaling(), offspring.second);

      return offspring;
    }

  } // namespace details

  template<typename Range, typename Population>
  concept parents_range =
      selection_range<Range, typename Population::itreator_t>;

  template<typename Context>
  class exclusive {
  public:
    using context_t = Context;

  public:
    inline explicit exclusive(context_t const& context) {
    }

    template<typename Population, parents_range<Population> Parents>
    inline auto operator()(Population& population, Parents&& parents) {
    }

  private:
    context_t const* context_;
  };

  class overlapping {};

  class field {};

} // namespace couple
} // namespace gal
