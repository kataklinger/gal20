
#pragma once

#include "operation.hpp"
#include "utility.hpp"

#include <unordered_set>

namespace gal {
namespace couple {

  template<auto Crossover,
           auto Mutation,
           traits::boolean_flag MutationImproveOnly,
           typename Generator>
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

    template<typename Context, typename Params, traits::boolean_flag Pairing>
    class incubator {
    public:
      using context_t = Context;
      using params_t = Params;
      using pairing_t = Pairing;

      using population_t = typename Context::population_t;
      using statistics_t = typename Context::statistics_t;

      using chromosome_t = typename population_t::chromosome_t;
      using parent_iterator_t = typename population_t::iterator_t;
      using individual_t = typename population_t::individual_t;

      using parentship_t =
          gal::details::parentship<parent_iterator_t, individual_t>;

      inline static constexpr auto pairing = pairing_t::value;

    private:
      using improve_t = typename params_t::mutation_improve_only_t;

    public:
      inline incubator(context_t& context,
                       params_t const& params,
                       std::size_t size,
                       pairing_t /*unused*/)
          : context_{&context}
          , params_{&params}
          , statistics_{&context.history()} {
        results_.reserve(size);
      }

      inline void operator()(parent_iterator_t parent1,
                             parent_iterator_t parent2) {
        auto [child1, child2] =
            reproduce_impl(parent1->chromosome(), parent2->chromosome());

        if constexpr (pairing) {
          results_.emplace_back(parent1, std::move(child1));
          results_.emplace_back(parent2, std::move(child2));
        }
        else {
          if (child1.evaluation().raw() >= child2.evaluation().raw()) {
            results_.emplace_back(parent1, std::move(child1));
          }
          else {
            results_.emplace_back(parent1, std::move(child2));
          }
        }
      }

      inline auto&& take() noexcept {
        return std::move(results_);
      }

    private:
      auto reproduce_impl(chromosome_t const& parent1,
                          chromosome_t const& parent2) const {
        auto do_cross = std::invoke(params_->crossover());
        auto produced =
            do_cross ? std::invoke(context_->crossover(), parent1, parent2)
                     : std::pair{parent1, parent2};

        increment_if(do_cross, crossover_count_tag);

        std::pair offspring{try_mutate(produced.first),
                            try_mutate(produced.second)};

        if constexpr (has_scaling_v<population_t, context_t>) {
          std::invoke(context_->scaling(), offspring.first);
          std::invoke(context_->scaling(), offspring.second);
        }

        return offspring;
      }

      individual_t try_mutate(chromosome_t& original) const {
        auto do_mutate = std::invoke(params_->mutation());

        auto mutated =
            mutate(context_->mutation(),
                   traits::move_if(original, std::negation<improve_t>{}),
                   do_mutate);

        auto mutated_fitness = std::invoke(context_->evaluator(), mutated);

        increment_if(do_mutate, mutation_tried_count_tag);

        if constexpr (improve_t::value) {
          if (do_mutate) {
            auto original_fitness =
                std::invoke(context_->evaluator(), original);

            if (original_fitness > mutated_fitness) {
              increment(mutation_accepted_count_tag);
              return {std::move(original), std::move(original_fitness)};
            }
          }
        }
        else {
          increment_if(do_mutate, mutation_accepted_count_tag);
        }

        return {std::move(mutated), std::move(mutated_fitness)};
      }

      template<typename Mutation, typename Chromosome>
      inline chromosome_t mutate(Mutation const& mutation,
                                 Chromosome&& chromosome,
                                 bool do_mutate) const {
        if (do_mutate) {
          if constexpr (std::is_lvalue_reference_v<Chromosome>) {
            chromosome_t mutated{chromosome};
            std::invoke(mutation, mutated);
            return mutated;
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
      inline void increment(Tag tag) const {
        stat::increment_count(get_stats(), tag);
      }

      template<typename Tag>
      inline void increment_if(bool executed, Tag tag) const {
        stat::increment_count<Tag>(get_stats(), tag, {executed});
      }

      inline auto& get_stats() const noexcept {
        return statistics_->current();
      }

    private:
      context_t* context_;
      params_t const* params_;
      stat::history<statistics_t>* statistics_;

      std::vector<parentship_t> results_;
    };

  } // namespace details

  template<typename Range, typename Population>
  concept parents_range =
      selection_range<Range, typename Population::iterator_t>;

  template<typename Context, typename Params>
  class exclusive {
  public:
    using context_t = Context;
    using params_t = Params;

    using population_t = typename context_t::population_t;

  public:
    inline explicit exclusive(context_t& context, params_t const& params)
        : context_{&context}
        , params_{params} {
    }

    template<parents_range<population_t> Parents>
    inline auto operator()(Parents&& parents) {
      details::incubator incubate{
          *context_, params_, std::ranges::size(parents), std::true_type{}};

      auto it = std::ranges::begin(parents);
      if (auto size = std::ranges::size(parents); size % 2 == 0) {
        for (auto end = it + (size - 1); it != end; it += 2) {
          incubate(*it, *(it + 1));
        }
      }
      else {
        for (auto end = std::ranges::end(parents); it != end; it += 2) {
          incubate(*it, *(it + 1));
        }
      }

      return incubate.take();
    }

  private:
    context_t* context_;
    params_t params_;
  };

  template<typename Context, typename Params>
  class overlapping {
  public:
    using context_t = Context;
    using params_t = Params;

    using population_t = typename context_t::population_t;

  public:
    inline explicit overlapping(context_t& context, params_t const& params)
        : context_{&context}
        , params_{params} {
    }

    template<parents_range<population_t> Parents>
    inline auto operator()(Parents&& parents) {
      details::incubator incubate{
          *context_, params_, std::ranges::size(parents), std::false_type{}};

      auto fst = std::ranges::begin(parents), prev = fst, cur = prev + 1;
      for (; cur != std::ranges::end(parents); prev = cur++) {
        incubate(*prev, *cur);
      }

      incubate(*prev, *fst);

      return incubate.take();
    }

  private:
    context_t* context_;
    params_t params_;
  };

  template<typename Context, typename Params>
  class field {
  public:
    using context_t = Context;
    using params_t = Params;

    using population_t = typename context_t::population_t;

  private:
    using parent_t = typename population_t::individual_t*;

  public:
    inline explicit field(context_t& context, params_t const& params)
        : context_{&context}
        , params_{params} {
    }

    template<parents_range<population_t> Parents>
    inline auto operator()(Parents&& parents) {
      auto count = std::ranges::size(parents);

      details::incubator incubate{
          *context_, params_, count * (count - 1), std::true_type{}};

      auto end = std::ranges::end(parents);
      for (auto fst = std::ranges::begin(parents); fst != end; ++fst) {
        for (auto snd = fst + 1; fst != end; ++snd) {
          incubate(*fst, *snd);
        }
      }

      auto all = incubate.take();
      std::ranges::sort(all, std::ranges::greater{}, [](auto const& item) {
        return get_child(item).evaluation().raw();
      });

      decltype(all) results{};
      results.reserve(count);

      std::unordered_set<parent_t> processed{};
      processed.reserve(count);

      for (auto& item : all) {
        if (auto [it, fresh] = processed.insert(&*get_parent(item)); fresh) {
          results.push_back(std::move(item));
        }
      }

      return results;
    }

  private:
    context_t* context_;
    params_t params_;
  };

  template<template<typename, typename> class Coupling, typename Params>
  class factory {
  public:
    using params_t = Params;

  public:
    inline constexpr explicit factory(params_t const& params)
        : params_{params} {
    }

    template<typename Context>
    inline constexpr auto operator()(Context& context) const {
      return Coupling<Context, params_t>{context, params_};
    }

  private:
    params_t params_;
  };

  template<template<typename, typename> class Coupling, typename Params>
  inline constexpr auto make_factory(Params const& params) {
    return factory<Coupling, Params>{params};
  }

} // namespace couple
} // namespace gal
