
#include "stats.hpp"

namespace gal {
namespace config {

  template<typename Fragment>
  concept fragment = !std::is_final_v<Fragment> && std::copyable<Fragment>;

  namespace details {

    class null_config {};
    class null_iface {};
    class null_builder {};

    template<fragment... Fragments>
    class config_node;

    template<>
    class config_node<> {};

    template<fragment Fragment>
    class config_node<Fragment> : public Fragment {
    public:
      using fragment_t = Fragment;

    public:
      constexpr inline config_node(fragment_t const& fragment,
                                   config_node<> const& /*unused*/)
          : fragment_t{fragment} {
      }
    };

    template<fragment Fragment, fragment... Fragments>
    class config_node<Fragment, Fragments...>
        : public Fragment, public config_node<Fragments...> {
    public:
      using fragment_t = Fragment;
      using base_t = config_node<Fragments...>;

    public:
      constexpr inline config_node(fragment_t const& frag, base_t const& base)
          : fragment_t{frag}
          , base_t{base} {
      }
    };

    template<template<typename> class... Interfaces>
    struct iflist {};

    template<typename Left, typename Right>
    struct iflist_merge;

    template<template<typename> class... Lefts,
             template<typename>
             class... Rights>
    struct iflist_merge<iflist<Lefts...>, iflist<Rights...>> {
      using type = iflist<Lefts..., Rights...>;
    };

    template<typename Left, typename Right>
    using iflist_merge_t = typename iflist_merge<Left, Right>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct iflist_add;

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct iflist_add<Interface, iflist<Interfaces...>> {
      using type = iflist<Interface, Interfaces...>;
    };

    template<template<typename> class Interface, typename Interfaces>
    using iflist_add_t = typename iflist_add<Interface, Interfaces>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct iflist_remove;

    template<template<typename> class Interface>
    struct iflist_remove<Interface, iflist<>> {
      using type = iflist<>;
    };

    template<template<typename> class Interface,
             template<typename>
             class Current,
             template<typename>
             class... Interfaces>
    struct iflist_remove<Interface, iflist<Current, Interfaces...>>
        : iflist_remove<Interface, iflist<Interfaces...>> {
      using type = iflist_add_t<
          Current,
          typename iflist_remove<Interface, iflist<Interfaces...>>::type>;
    };

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct iflist_remove<Interface, iflist<Interface, Interfaces...>> {
      using type = iflist<Interfaces...>;
    };

    template<template<typename> class Interface, typename Interfaces>
    using iflist_remove_t = typename iflist_remove<Interface, Interfaces>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct iflist_contain;

    template<template<typename> class Interface>
    struct iflist_contain<Interface, iflist<>> : std::false_type {
      using type = iflist<>;
    };

    template<template<typename> class Interface,
             template<typename>
             class Current,
             template<typename>
             class... Interfaces>
    struct iflist_contain<Interface, iflist<Current, Interfaces...>>
        : iflist_contain<Interface, iflist<Interfaces...>> {};

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct iflist_contain<Interface, iflist<Interface, Interfaces...>>
        : std::true_type {};

    template<template<typename> class Interface, typename Interfaces>
    constexpr inline auto iflist_contain_v =
        iflist_contain<Interface, Interfaces>::value;

    template<typename Used, typename Required>
    struct iface_satisfied_helper : std::false_type {};

    template<typename Used, template<typename> class... Required>
    struct iface_satisfied_helper<Used, details::iflist<Required...>>
        : std::conjunction<details::iflist_contain<Required, Used>...> {};

    template<typename Used, typename Interface, typename = void>
    struct is_iface_satisfied : std::true_type {};

    template<typename Used, typename Interface>
    struct is_iface_satisfied<Used,
                              Interface,
                              std::void_t<typename Interface::required_t>>
        : iface_satisfied_helper<Used, typename Interface::required_t> {};

    template<typename Used, typename Interface>
    constexpr inline auto is_iface_satisfied_v =
        is_iface_satisfied<Used, Interface>;

    template<typename Used, typename Interface>
    struct iface_inherit
        : std::conditional<is_iface_satisfied_v<Used, Interface>,
                           Interface,
                           null_iface> {};

    template<typename Used, typename Interface>
    using iface_inherit_t = typename iface_inherit<Used, Interface>::type;

    template<typename Builder, typename Used, typename Interfaces>
    class iface_node {};

    template<typename Builder, typename Used>
    class iface_node<Builder, Used, details::iflist<>> {};

    template<typename Builder,
             typename Used,
             template<typename>
             class Interface,
             template<typename>
             class... Interfaces>
    class iface_node<Builder, Used, details::iflist<Interface, Interfaces...>>
        : public iface_inherit_t<Used, Interface<Builder>>,
          public iface_node<Builder, Used, details::iflist<Interfaces...>> {};

    template<typename Available, typename Used, fragment... Fragments>
    class builder_node;

    template<typename Available, typename Used>
    class builder_node<Available, Used> {};

    template<typename Available,
             typename Used,
             fragment Fragment,
             fragment... Rest>
    class builder_node<Available, Used, Fragment, Rest...>
        : public iface_node<builder_node<Available, Used, Fragment, Rest...>,
                            Used,
                            Available>,
          public config_node<Fragment, Rest...> {
    public:
      using fragment_t = Fragment;
      using base_t = config_node<fragment_t, Rest...>;
      using previous_t = config_node<Rest...>;

    public:
      constexpr inline builder_node(fragment_t const& fragment,
                                    previous_t const& previous)
          : base_t{fragment, previous} {
      }

      constexpr inline auto build() const {
        return static_cast<base_t>(*this);
      }
    };

    template<typename Builder,
             fragment Fragment,
             template<typename>
             class This,
             typename Added>
    struct next_builder;

    template<typename Available,
             typename Used,
             typename... Fragments,
             fragment Fragment,
             template<typename>
             class This,
             typename Added>
    struct next_builder<builder_node<Available, Used, Fragments...>,
                        Fragment,
                        This,
                        Added> {
      using type = builder_node<
          details::iflist_remove_t<This,
                                   details::iflist_merge_t<Added, Available>>,
          details::iflist_add_t<This, Used>,
          Fragment,
          Fragments...>;
    };

    template<typename Builder,
             template<typename>
             class Current,
             fragment Fragment,
             typename Added>
    using next_builder_t =
        typename next_builder<Builder, Fragment, Current, Added>::type;

    template<typename Builder, template<typename> class Derived>
    struct iface_base {
      template<typename Added, fragment Fragment>
      constexpr inline auto next(Fragment&& fragment) const {
        return next_builder_t<Builder, Derived, Fragment, Added>{
            static_cast<Builder const&>(*this),
            std::forward<Fragment>(fragment)};
      }

      template<template<typename> class... Added, fragment Fragment>
      constexpr inline auto next(Fragment&& fragment) const {
        return next<details::iflist<Added...>,
                    std::forward<Fragment>(fragment)>();
      }
    };

  } // namespace details

  template<typename Population, typename Statistics>
  class population_context {
  public:
    using population_t = Population;
    using statistics_t = Statistics;

  public:
    constexpr inline population_context(population_t& population,
                                        statistics_t& statistics) noexcept
        : population_{&population}
        , statistics_{&statistics} {
    }

    inline population_t& population() noexcept {
      return *population_;
    }

    inline population_t const& population() const noexcept {
      return *population_;
    }

    inline statistics_t& statistics() noexcept {
      return *statistics_;
    }

    inline statistics_t const& statistics() const noexcept {
      return *statistics_;
    }

  private:
    population_t* population_;
    statistics_t* statistics_;
  };

  template<typename Crossover,
           typename Mutation,
           typename Evaluator,
           typename ImprovingMutation>
  class reproduction_context {
  public:
    using crossover_t = Crossover;
    using mutation_t = Mutation;
    using evaluator_t = Evaluator;

    using improving_mutation_t = ImprovingMutation;

  public:
    constexpr inline reproduction_context(crossover_t const& crossover,
                                          mutation_t const& mutation,
                                          evaluator_t const& evaluator) noexcept
        : crossover_{&crossover}
        , mutation_{&mutation}
        , evaluator_{evaluator} {
    }

    inline crossover_t const& crossover() const noexcept {
      return crossover_;
    }

    inline mutation_t const& mutation() const noexcept {
      return mutation_;
    }

    inline evaluator_t const& evaluator() const noexcept {
      return evaluator_;
    }

  private:
    crossover_t crossover_;
    mutation_t mutation_;
    evaluator_t evaluator_;
  };

  template<typename Crossover,
           typename Mutation,
           typename Evaluator,
           typename ImprovingMutation,
           typename Scaling>
  class reproduction_context_with_scaling
      : public reproduction_context<Crossover,
                                    Mutation,
                                    Evaluator,
                                    ImprovingMutation> {
  public:
    using crossover_t = Crossover;
    using mutation_t = Mutation;
    using evaluator_t = Evaluator;
    using scaling_t = Scaling;

  public:
    constexpr inline reproduction_context_with_scaling(
        crossover_t const& crossover,
        mutation_t const& mutation,
        evaluator_t const& evaluator,
        scaling_t const& scaling) noexcept
        : reproduction_context_with_scaling{crossover, mutation, evaluator}
        , scaling_{scaling} {
    }

    inline scaling_t const& scaling() const noexcept {
      return scaling_;
    }

  private:
    scaling_t scaling_;
  };

  template<typename Builder, typename = typename Builder::is_global_scaling_t>
  struct build_reproduction_context {
    using type = reproduction_context<typename Builder::crossover_t,
                                      typename Builder::mutation_t,
                                      typename Builder::evaluator_t,
                                      typename Builder::improving_mutation_t>;
  };

  template<typename Builder>
  struct build_reproduction_context<Builder, std::false_type> {
    using type = reproduction_context_with_scaling<
        typename Builder::crossover_t,
        typename Builder::mutation_t,
        typename Builder::evaluator_t,
        typename Builder::improving_mutation_t,
        typename Builder::scaling_t>;
  };

  template<typename Builder>
  using build_reproduction_context_t =
      typename build_reproduction_context<Builder>::type;

  template<typename Builder>
  struct tags_iface : private details::iface_base<Builder, tags_iface> {
    template<typename Tags>
    constexpr inline auto tag_with() const {
      struct node {
        using tags_t = Tags;
      };

      return this->template next<>(node{});
    }

    constexpr inline auto tag_nothing() const {
      struct node {
        using tags_t = empty_tags;
      };

      return this->template next<>(node{});
    }
  };

  template<typename Builder>
  struct criterion_iface
      : private details::iface_base<Builder, criterion_iface> {
    using population_t = typename Builder::population_t;
    using statistics_t = typename Builder::statistics_t;

  public:
    template<criterion<population_t, statistics_t> Criterion>
    constexpr inline auto stop_when(Criterion const& criterion) const {
      class node {
      public:
        using criterion_t = Criterion;

      public:
        constexpr inline explicit node(criterion_t const& criterion)
            : criterion_{criterion} {
        }

        inline auto criterion() const {
          return criterion_;
        }

      private:
        criterion_t criterion_;
      };

      return this->template next<>(node{criterion});
    }
  };

  template<typename Builder>
  struct replace_iface : private details::iface_base<Builder, replace_iface> {
    using population_t = typename Builder::population_t;
    using copuling_result_t = typename Builder::copuling_result_t;

    template<replacement<population_t, copuling_result_t> Replacement>
    constexpr inline auto replace_with(Replacement const& replacement) const {
      class node {
      public:
        using replacement_t = Replacement;

      public:
        constexpr inline explicit node(replacement_t const& replacement)
            : replacement_{replacement} {
        }

        inline auto const& replacement() const noexcept {
          return replacement_;
        }

      private:
        replacement_t replacement_;
      };

      return this->template next<>(node{replacement});
    }
  };

  template<typename Builder>
  struct couple_iface : private details::iface_base<Builder, couple_iface> {
    using reproduction_context_t = build_reproduction_context_t<Builder>;
    using selection_result_t = typename Builder::selection_result_t;

    template<
        coupling_factory<reproduction_context_t, selection_result_t> Factory>
    constexpr inline auto couple_like(Factory const& coupling) const {
      using factory_t = Factory;

      class node {
      public:
        using coupling_t = factory_result_t<factory_t, reproduction_context_t>;
        using copuling_result_t = std::invoke_result_t<
            coupling_t,
            std::add_lvalue_reference_t<std::add_const_t<selection_result_t>>>;

      public:
        constexpr inline explicit node(factory_t const& coupling)
            : coupling_{coupling} {
        }

        inline auto coupling(reproduction_context_t& context) const {
          return coupling_(context);
        }

      private:
        factory_t coupling_;
      };

      return this->template next<replace_iface>(node{coupling});
    }
  };

  template<typename Builder>
  struct select_iface : private details::iface_base<Builder, select_iface> {
    using population_t = typename Builder::population_t;

    template<selection<population_t> Selection>
    constexpr inline auto select_using(Selection const& selection) const {
      class node {
      public:
        using selection_t = Selection;
        using selection_result_t = std::invoke_result_t<
            selection_t,
            std::add_lvalue_reference_t<std::add_const_t<population_t>>>;

      public:
        constexpr inline explicit node(selection_t const& selection)
            : selection_{selection} {
        }

        inline auto const& selection() const noexcept {
          return selection_;
        }

      private:
        selection_t selection_;
      };

      return this->template next<couple_iface>(node{selection});
    }
  };

  template<typename Builder>
  struct scale_iface : private details::iface_base<Builder, scale_iface> {
    using required_t = details::iflist<tags_iface>;
    using population_context_t = typename Builder::population_context_t;

    template<scaling_factory<population_context_t> Factory>
    constexpr inline auto scale_as(Factory const& scaling) const {
      using factory_t = Factory;
      using chromosome_t = typename Builder::chromosome_t;
      using raw_fitness_t = typename Builder::raw_fitness_t;

      class node {
      public:
        using scaling_t = factory_result_t<factory_t, population_context_t>;
        using is_global_scaling_t =
            typename scaling_traits<scaling_t>::is_global_t;

      public:
        constexpr inline explicit node(factory_t const& scaling)
            : scaling_{scaling} {
        }

        inline auto scaling(population_context_t& context) const {
          return scaling_(context);
        }

      private:
        factory_t scaling_;
      };

      return this->template next<select_iface>(node{scaling});
    }
  };

  template<typename Builder>
  struct reproduce_iface
      : private details::iface_base<Builder, reproduce_iface> {
    template<crossover<typename Builder::chromosome_t> Crossover,
             mutation<typename Builder::chromosome_t> Mutation,
             bool ImprovingMutation = false>
    constexpr inline auto reproduce_using(
        Crossover const& crossover,
        Mutation const& mutation,
        std::bool_constant<ImprovingMutation> /*unused*/) const {
      using chromosome_t = typename Builder::chromosome_t;

      class node {
      public:
        using crossover_t = Crossover;
        using mutation_t = Mutation;

        using improving_mutation_t = std::bool_constant<ImprovingMutation>;

      public:
        constexpr inline explicit node(crossover_t const& crossover,
                                       mutation_t const& mutation)
            : crossover_{crossover}
            , mutation_{mutation} {
        }

        inline auto const& crossover() const noexcept {
          return crossover_;
        }

        inline auto const& mutation() const noexcept {
          return mutation_;
        }

      private:
        crossover_t crossover_;
        mutation_t mutation_;
      };

      return this->template next<>(node{crossover, mutation});
    }
  };

  template<typename Builder>
  struct statistics_iface
      : private details::iface_base<Builder, statistics_iface> {
    template<stats::node_value<typename Builder::population_t>... Values>
    constexpr inline auto track_these() const {
      using population_t = typename Builder::population_t;

      struct node {
        using statistics_t = stats::statistics<population_t, Values...>;
        using population_context_t =
            population_context<population_t, statistics_t>;
      };

      using added_t = std::conditional_t<
          std::is_same_v<empty_fitness, typename Builder::scaled_fitness_t>,
          details::iflist<select_iface, criterion_iface>,
          details::iflist<scale_iface, criterion_iface>>;

      return this->template next<added_t>(node{});
    }
  };

  template<typename Builder>
  class scale_fitness_iface
      : private details::iface_base<Builder, scale_fitness_iface> {
  private:
    struct node_base {};
    struct node_base_none {
      using is_global_scaling_t = std::true_type;
    };

    template<fitness Scaled>
    constexpr inline auto scale_to() const {
      return scale_impl<Scaled, node_base>();
    }

    constexpr inline auto scale_none() const {
      return scale_impl<empty_fitness, node_base_none>();
    }

  private:
    template<fitness Scaled, typename Base>
    constexpr inline auto scale_impl() const {
      struct node : Base {
        using scaled_fitness_t = Scaled;
        using population_t = population<typename Builder::chromosome_t,
                                        typename Builder::raw_fitness_t,
                                        scaled_fitness_t,
                                        typename Builder::tags_t>;
      };

      return this->template next<statistics_iface>(node{});
    }
  };

  template<typename Builder>
  struct evaluate_iface : private details::iface_base<Builder, evaluate_iface> {
    template<evaluator<typename Builder::chromosome_t> Evaluator>
    constexpr inline auto evaluate_against(Evaluator const& evaluator) const {
      using chromosome_t = typename Builder::chromosome_t;

      class node {
      public:
        using evaluator_t = Evaluator;
        using raw_fitness_t = std::invoke_result_t<
            evaluator_t,
            std::add_lvalue_reference_t<std::add_const_t<chromosome_t>>>;

      public:
        constexpr inline explicit node(evaluator_t const& evaluator)
            : evaluator_{evaluator} {
        }

        inline auto const& evaluator() const noexcept {
          return evaluator_;
        }

      private:
        evaluator_t evaluator_;
      };

      return this->template next<scale_fitness_iface>(node{evaluator});
    }
  };

  template<typename Builder>
  struct init_iface : private details::iface_base<Builder, init_iface> {
    template<initializator Initializator>
    constexpr inline auto make_like(Initializator const& initializator) const {
      class node {
      public:
        using initializator_t = Initializator;
        using chromosome_t = std::invoke_result_t<initializator_t>;

      public:
        constexpr inline explicit node(initializator_t const& initializator)
            : initializator_{initializator} {
        }

        inline auto const& initializator() const noexcept {
          return initializator_;
        }

      private:
        initializator_t initializator_;
      };

      return this->template next<evaluate_iface, reproduce_iface>(
          node{initializator});
    }
  };

  template<typename Builder>
  struct root_iface : public details::iface_base<Builder, root_iface> {
    constexpr inline auto begin() const {
      return this->template next<init_iface, tags_iface>(
          details::null_config{});
    }
  };

  template<template<typename> class Root>
  using builder = details::builder_node<details::iflist<Root>,
                                        details::iflist<>,
                                        details::null_builder>;

} // namespace config
} // namespace gal
