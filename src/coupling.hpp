
#pragma once

#include "operation.hpp"
#include "utility.hpp"

namespace gal {
namespace couple {

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
      return reproduce_impl(context, parent1, parent2, params);
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

    template<typename Context, typename Params>
    class incubator {
    public:
      using context_t = Context;
      using params_t = Params;

    public:
      inline incubator(context_t const& context,
                       params_t const& params,
                       std::size_t size)
          : context_{&context}
          , params_{&params} {
        results_.reserve(size);
      }

      template<std::input_iterator It>
      void reproduce(It parent1, It parent2) const {
        auto offspring = reproduce(*context_, *parent1, *parent2, *params_);
        results_.emplace_back(parent1, std::move(offspring.first));
        results_.emplace_back(parent2, std::move(offspring.second));
      }

      auto&& take() noexcept {
        return std::move(results_);
      }

    private:
      context_t const* context_;
      params_t const* params_;

      std::vector<gal::details::parentship> results_;
    };

  } // namespace details

  template<typename Range, typename Population>
  concept parents_range =
      selection_range<Range, typename Population::itreator_t>;

  template<typename Population, typename Context, typename Params>
  class exclusive {
  public:
    using population_t = Population;
    using context_t = Context;
    using params_t = Params;

  public:
    inline explicit exclusive(context_t const& context, params_t const& params)
        : context_{&context}
        , params_{params} {
    }

    template<parents_range<population_t> Parents>
    inline auto operator()(Parents&& parents) {
      details::incubator incubate{
          *context_, params_, std::ranges::size(parents)};

      for (auto it = std::ranges::begin(parents);
           it != std::ranges::end(parents);
           it += 2) {
        incubate(*it, *(it + 1));
      }

      return incubate.take();
    }

  private:
    context_t const* context_;
    params_t params_;
  };

  class overlapping {};

  class field {};

} // namespace couple
} // namespace gal
