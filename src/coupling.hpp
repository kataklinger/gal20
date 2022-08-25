
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

    template<typename Context, typename Params>
    class incubator {
    public:
      using context_t = Context;
      using params_t = Params;

      using population_t = typename Context::population_t;
      using statistics_t = typename Context::statistics_t;

      using chromosome_t = typename population_t::chromosome_t;
      using parent_iterator_t = typename population_t::iterator_t;
      using individual_t = typename population_t::individual_t;

      using parentship_t =
          gal::details::parentship<parent_iterator_t, individual_t>;

    private:
      using improve_t = typename params_t::mutation_improve_only_t;

    public:
      inline incubator(context_t& context,
                       params_t const& params,
                       std::size_t size,
                       statistics_t& statistics_)
          : context_{&context}
          , params_{&params} {
        results_.reserve(size);
      }

      inline void reproduce(parent_iterator_t parent1,
                            parent_iterator_t parent2) const {
        auto offspring =
            reproduce_impl(*context_, *parent1, *parent2, *params_);
        results_.emplace_back(parent1, std::move(offspring.first));
        results_.emplace_back(parent2, std::move(offspring.second));
      }

      inline auto&& take() noexcept {
        return std::move(results_);
      }

    private:
      auto reproduce_impl(chromosome_t const& parent1,
                          chromosome_t const& parent2) {
        auto do_cross = std::invoke(params_->crossover());
        auto produced =
            do_cross ? std::invoke(context_->crossover(), parent1, parent2)
                     : std::pair{parent1, parent2};

        increment_if<stat::crossover_count_t>(do_cross);

        std::pair offspring{try_mutate(produced.first),
                            try_mutate(produced.second)};

        if constexpr (has_scaling_v<population_t, context_t>) {
          std::invoke(context_->scaling(), offspring.first);
          std::invoke(context_->scaling(), offspring.second);
        }

        return offspring;
      }

      individual_t try_mutate(chromosome_t& original) {
        auto do_mutate = std::invoke(params_->mutation());

        auto mutated =
            mutate(context_->mutation(),
                   traits::move_if(original, std::negation<improve_t>{}),
                   do_mutate);

        auto mutated_fitness = std::invoke(context_->evaluator(), mutated);

        increment_if<stat::mutation_tried_count_t>(do_mutate);

        if constexpr (improve_t::value) {
          if (do_mutate) {
            auto original_fitness =
                std::invoke(context_->evaluator(), original);

            if (original_fitness > mutated_fitness) {
              increment<stat::mutation_accepted_count_t>();
              return {std::move(original), {std::move(original_fitness)}};
            }
          }
        }
        else {
          increment_if<stat::mutation_accepted_count_t>(do_mutate);
        }

        return {std::move(mutated), {std::move(mutated_fitness)}};
      }

      template<typename Mutation, typename Chromosome, typename Probability>
      inline individual_t mutate(Mutation const& mutation,
                                 Chromosome&& chromosome,
                                 bool do_mutate) {
        if (do_mutate) {
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

      template<typename Tag>
      inline void increment() {
        stat::increment_count<Tag>(*statistics_);
      }

      template<typename Tag>
      inline void increment_if(bool executed) {
        stat::increment_count<Tag>(*statistics_, {executed});
      }

    private:
      context_t* context_;
      params_t const* params_;
      statistics_t* statistics_;

      std::vector<parentship_t> results_;
    };

  } // namespace details

  template<typename Range, typename Population>
  concept parents_range =
      selection_range<Range, typename Population::itreator_t>;

  template<typename Context, typename Params>
  class exclusive {
  public:
    using context_t = Context;
    using params_t = Params;

    using population_t = typename context_t::population_t;

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

  template<template<typename, typename> class Coupling, typename Params>
  class factory {
  public:
    using params_t = Params;

  public:
    inline explicit factory(params_t const& params)
        : params_{params} {
    }

    template<typename Context>
    auto operator()(Context& context) {
      return Coupling<Context, params_t>{context, params_};
    }

  private:
    params_t params_;
  };

  template<template<typename, typename> class Coupling, typename Params>
  inline auto make_factory(Params const& params) {
    return factory<Coupling, Params>{params};
  }

} // namespace couple
} // namespace gal
