
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

    template<typename Population, typename Context>
    struct has_scaling : std::false_type {};

    template<typename Population, with_scaling<Population> Context>
    struct has_scaling<Context, Population> : std::true_type {};

    template<typename Population, typename Context>
    inline constexpr auto has_scaling_v =
        has_scaling<Context, Population>::value;

    template<typename Population, typename Context, typename Params>
    class incubator {
    public:
      using population_t = Population;
      using context_t = Context;
      using params_t = Params;

      using chromosome_t = typename population_t::chromosome_t;
      using parent_iterator_t = typename population_t::iterator_t;
      using individual_t = typename population_t::individual_t;

      using parentship_t =
          gal::details::parentship<parent_iterator_t, individual_t>;

    private:
      using improve_t = typename params_t::mutation_improve_only_t;

    public:
      inline incubator(context_t const& context,
                       params_t const& params,
                       std::size_t size)
          : context_{&context}
          , params_{&params} {
        results_.reserve(size);
      }

      void reproduce(parent_iterator_t parent1,
                     parent_iterator_t parent2) const {
        auto offspring =
            reproduce_impl(*context_, *parent1, *parent2, *params_);
        results_.emplace_back(parent1, std::move(offspring.first));
        results_.emplace_back(parent2, std::move(offspring.second));
      }

      auto&& take() noexcept {
        return std::move(results_);
      }

    private:
      inline auto reproduce_impl(chromosome_t const& parent1,
                                 chromosome_t const& parent2) {
        auto crossed =
            std::invoke(params_->crossover())
                ? std::invoke(context_->crossover(), parent1, parent2)
                : std::pair{parent1, parent2};

        std::pair offspring{try_mutate(crossed.first),
                            try_mutate(crossed.second)};

        if constexpr (has_scaling_v<population_t, context_t>) {
          std::invoke(context_->scaling(), offspring.first);
          std::invoke(context_->scaling(), offspring.second);
        }

        return offspring;
      }

      individual_t try_mutate(chromosome_t& original) {
        auto mutated =
            mutate(context_->mutation(),
                   traits::move_if(original, std::negation<improve_t>{}),
                   params_->mutation());

        auto mutated_fitness = std::invoke(context_->evaluator(), mutated);

        if constexpr (improve_t::value) {
          auto original_fitness = std::invoke(context_->evaluator(), original);

          if (original_fitness > mutated_fitness) {
            return {std::move(original), {std::move(original_fitness)}};
          }
        }

        return {std::move(mutated), {std::move(mutated_fitness)}};
      }

      template<typename Mutation, typename Chromosome, typename Probability>
      individual_t mutate(Mutation const& mutation,
                          Chromosome&& chromosome,
                          Probability const& probability) {
        if (std::invoke(probability)) {
          if constexpr (std::is_lvalue_reference_v<Chromosome>) {
            individual_t mutated{chromosome};
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

    private:
      context_t const* context_;
      params_t const* params_;

      std::vector<parentship_t> results_;
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
      details::incubator<population_t, context_t, params_t> incubate{
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
