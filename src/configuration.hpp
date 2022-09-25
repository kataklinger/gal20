
#pragma once

#include "context.hpp"
#include "statistic.hpp"

namespace gal {
namespace config {

  template<typename Section>
  concept section = !std::is_final_v<Section> && std::copyable<Section>;

  template<template<typename> class... Interfaces>
  struct plist {};

  template<template<typename> class Key,
           typename Unlocked,
           typename Required = plist<>>
  struct entry {};

  template<typename... Maps>
  struct entry_map {};

  template<template<typename> class Condition, typename Then, typename Else>
  struct entry_if {};

  namespace details {

    template<typename Entry, typename Built>
    struct eval_entry_if {
      using type = Entry;
    };

    template<template<typename> class Condition,
             typename Then,
             typename Else,
             typename Built>
    struct eval_entry_if<entry_if<Condition, Then, Else>, Built> {
      using type = std::conditional_t<Condition<Built>::value, Then, Else>;
    };

    template<typename Entry, typename Built>
    using eval_entry_if_t = typename eval_entry_if<Entry, Built>::type;

    template<typename Map, template<typename> class Match, typename Built>
    struct entry_map_match;

    template<template<typename> class Match, typename Built>
    struct entry_map_match<entry_map<>, Match, Built> {
      using unlocked_t = plist<>;
      using required_t = plist<>;
    };

    template<typename Unlocked,
             typename Required,
             typename... Rest,
             template<typename>
             class Match,
             typename Built>
    struct entry_map_match<entry_map<entry<Match, Unlocked, Required>, Rest...>,
                           Match,
                           Built> {
      using unlocked_t = eval_entry_if_t<Unlocked, Built>;
      using required_t = eval_entry_if_t<Required, Built>;
    };

    template<typename Entry,
             typename... Rest,
             template<typename>
             class Match,
             typename Built>
    struct entry_map_match<entry_map<Entry, Rest...>, Match, Built>
        : entry_map_match<entry_map<Rest...>, Match, Built> {};

    class empty_section {};
    class empty_ptype {};

    template<section... Sections>
    class section_node;

    template<>
    class section_node<> {};

    template<>
    class section_node<details::empty_section> : public details::empty_section {
    public:
      using section_t = details::empty_section;

      constexpr inline section_node() noexcept {
      }

      constexpr inline section_node(section_t const& section,
                                    section_node<> const& /*unused*/) noexcept {
      }
    };

    template<section Section>
    class section_node<Section> : public Section {
    public:
      using section_t = Section;

    public:
      constexpr inline section_node(section_t const& section,
                                    section_node<> const& /*unused*/)
          : section_t{section} {
      }
    };

    template<section Section, section... Sections>
    class section_node<Section, Sections...>
        : public Section, public section_node<Sections...> {
    public:
      using section_t = Section;
      using base_t = section_node<Sections...>;

    public:
      constexpr inline section_node(section_t const& section,
                                    base_t const& base)
          : section_t{section}
          , base_t{base} {
      }
    };

    template<typename Left, typename Right>
    struct plist_merge;

    template<template<typename> class... Lefts,
             template<typename>
             class... Rights>
    struct plist_merge<plist<Lefts...>, plist<Rights...>> {
      using type = plist<Lefts..., Rights...>;
    };

    template<typename Left, typename Right>
    using plist_merge_t = typename plist_merge<Left, Right>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct plist_add;

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct plist_add<Interface, plist<Interfaces...>> {
      using type = plist<Interface, Interfaces...>;
    };

    template<template<typename> class Interface, typename Interfaces>
    using plist_add_t = typename plist_add<Interface, Interfaces>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct plist_remove;

    template<template<typename> class Interface>
    struct plist_remove<Interface, plist<>> {
      using type = plist<>;
    };

    template<template<typename> class Interface,
             template<typename>
             class Current,
             template<typename>
             class... Interfaces>
    struct plist_remove<Interface, plist<Current, Interfaces...>>
        : plist_remove<Interface, plist<Interfaces...>> {
      using type = plist_add_t<
          Current,
          typename plist_remove<Interface, plist<Interfaces...>>::type>;
    };

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct plist_remove<Interface, plist<Interface, Interfaces...>> {
      using type = plist<Interfaces...>;
    };

    template<template<typename> class Interface, typename Interfaces>
    using plist_remove_t = typename plist_remove<Interface, Interfaces>::type;

    template<template<typename> class Interface, typename Interfaces>
    struct plist_contain;

    template<template<typename> class Interface>
    struct plist_contain<Interface, plist<>> : std::false_type {
      using type = plist<>;
    };

    template<template<typename> class Interface,
             template<typename>
             class Current,
             template<typename>
             class... Interfaces>
    struct plist_contain<Interface, plist<Current, Interfaces...>>
        : plist_contain<Interface, plist<Interfaces...>> {};

    template<template<typename> class Interface,
             template<typename>
             class... Interfaces>
    struct plist_contain<Interface, plist<Interface, Interfaces...>>
        : std::true_type {};

    template<template<typename> class Interface, typename Interfaces>
    constexpr inline auto plist_contain_v =
        plist_contain<Interface, Interfaces>::value;

    template<typename Used, typename Required>
    struct ptype_satisfied_helper : std::false_type {};

    template<typename Used, template<typename> class... Required>
    struct ptype_satisfied_helper<Used, plist<Required...>>
        : std::conjunction<plist_contain<Required, Used>...> {};

    template<typename Used, typename Interface, typename = void>
    struct is_ptype_satisfied : std::true_type {};

    template<typename Used, typename Interface>
    struct is_ptype_satisfied<Used,
                              Interface,
                              std::void_t<typename Interface::required_t>>
        : ptype_satisfied_helper<Used, typename Interface::required_t> {};

    template<typename Used, typename Interface>
    constexpr inline auto is_ptype_satisfied_v =
        is_ptype_satisfied<Used, Interface>::value;

    template<typename Used, typename Interface>
    struct ptype_inherit
        : std::conditional<is_ptype_satisfied_v<Used, Interface>,
                           Interface,
                           empty_ptype> {};

    template<typename Used, typename Interface>
    using ptype_inherit_t = typename ptype_inherit<Used, Interface>::type;

    template<typename Built, typename Used, typename Interfaces>
    class ptype_node {};

    template<typename Built, typename Used>
    class ptype_node<Built, Used, plist<>> {};

    template<typename Built,
             typename Used,
             template<typename>
             class Interface,
             template<typename>
             class... Interfaces>
    class ptype_node<Built, Used, plist<Interface, Interfaces...>>
        : public ptype_inherit_t<Used, Interface<Built>>,
          public ptype_node<Built, Used, plist<Interfaces...>> {};

    template<typename Entries,
             typename Available,
             typename Used,
             section... Sections>
    class builder_node;

    template<typename Entries,
             typename Available,
             typename Used,
             typename... Sections>
    class built : public section_node<Sections...> {
    public:
      using entries_t = Entries;
      using section_t = section_node<Sections...>;

      template<typename Builder>
      inline constexpr static decltype(auto)
          to_section(Builder const& builder) noexcept {
        if constexpr (std::is_same_v<section_t, section_node<>>) {
          return section_t{};
        }
        else {
          return static_cast<section_t const&>(builder);
        }
      }

      template<template<typename> class This, typename Added, typename Section>
      using builder_t =
          builder_node<entries_t,
                       plist_remove_t<This, plist_merge_t<Added, Available>>,
                       plist_add_t<This, Used>,
                       Section,
                       Sections...>;
    };

    template<typename Entries, typename Available, typename Used>
    class builder_node<Entries, Available, Used> {};

    template<typename Entries,
             typename Available,
             typename Used,
             section Section,
             section... Rest>
    class builder_node<Entries, Available, Used, Section, Rest...>
        : public ptype_node<built<Entries, Available, Used, Rest...>,
                            Used,
                            Available>,
          public section_node<Section, Rest...> {
    public:
      using section_t = Section;
      using base_t = section_node<section_t, Rest...>;
      using previous_t = section_node<Rest...>;

    public:
      template<
          typename = std::enable_if_t<std::is_same_v<section_t, empty_section>>>
      inline constexpr builder_node() noexcept {
      }

      inline constexpr builder_node(section_t const& section,
                                    previous_t const& previous)
          : base_t{section, previous} {
      }

      constexpr inline auto build() const {
        return static_cast<base_t>(*this);
      }
    };

    template<typename Built,
             section Section,
             template<typename>
             class This,
             typename Added>
    struct next_builder {
      using type = Built::template builder_t<This, Added, Section>;
    };

    template<typename Built,
             template<typename>
             class Current,
             section Section,
             typename Added>
    using next_builder_t =
        typename next_builder<Built, Section, Current, Added>::type;

    template<typename Built, template<typename> class Derived>
    class ptype_base {
    private:
      using entry_map_t =
          entry_map_match<typename Built::entries_t, Derived, Built>;

    public:
      using required_t = typename entry_map_t::required_t;

    protected:
      template<section Section>
      constexpr inline auto next(Section&& section) const {
        return next_builder_t<Built,
                              Derived,
                              Section,
                              typename entry_map_t::unlocked_t>{
            std::forward<Section>(section), Built::to_section(*this)};
      }
    };

    template<typename Built, typename = typename Built::is_global_scaling_t>
    struct build_reproduction_context {
      using type = reproduction_context<typename Built::population_t,
                                        typename Built::statistics_t,
                                        typename Built::crossover_t,
                                        typename Built::mutation_t,
                                        typename Built::evaluator_t>;
    };

    template<typename Built>
    struct build_reproduction_context<Built, std::false_type> {
      using type =
          reproduction_context_with_scaling<typename Built::population_t,
                                            typename Built::statistics_t,
                                            typename Built::crossover_t,
                                            typename Built::mutation_t,
                                            typename Built::evaluator_t,
                                            typename Built::scaling_t>;
    };

    template<typename Built>
    using build_reproduction_context_t =
        typename build_reproduction_context<Built>::type;

  } // namespace details

  template<typename Built>
  struct tags_ptype : public details::ptype_base<Built, tags_ptype> {
    template<typename Tags>
    constexpr inline auto tag_with() const {
      class body {
      public:
        using tags_t = Tags;
      };

      return this->template next<>(body{});
    }

    constexpr inline auto tag_nothing() const {
      class body {
      public:
        using tags_t = empty_tags;
      };

      return this->template next<>(body{});
    }
  };

  template<typename Built>
  struct criterion_ptype : public details::ptype_base<Built, criterion_ptype> {
    using population_t = typename Built::population_t;
    using statistics_t = typename Built::statistics_t;
    using history_t = stat::history<statistics_t>;

  public:
    template<criterion<population_t, history_t> Criterion>
    constexpr inline auto stop_when(Criterion const& criterion) const {
      class body {
      public:
        using criterion_t = Criterion;

      public:
        constexpr inline explicit body(criterion_t const& criterion)
            : criterion_{criterion} {
        }

        inline auto criterion() const {
          return criterion_;
        }

      private:
        criterion_t criterion_;
      };

      return this->template next<>(body{criterion});
    }
  };

  template<typename Built>
  struct replace_ptype : public details::ptype_base<Built, replace_ptype> {
    using population_t = typename Built::population_t;
    using copuling_result_t = typename Built::copuling_result_t;

    template<replacement<population_t, copuling_result_t> Replacement>
    constexpr inline auto replace_with(Replacement const& replacement) const {
      class body {
      public:
        using replacement_t = Replacement;

      public:
        constexpr inline explicit body(replacement_t const& replacement)
            : replacement_{replacement} {
        }

        inline auto const& replacement() const noexcept {
          return replacement_;
        }

      private:
        replacement_t replacement_;
      };

      return this->template next<>(body{replacement});
    }
  };

  template<typename Built>
  struct couple_ptype : public details::ptype_base<Built, couple_ptype> {
    using internal_reproduction_context_t =
        details::build_reproduction_context_t<Built>;
    using selection_result_t = typename Built::selection_result_t;

    template<coupling_factory<internal_reproduction_context_t,
                              selection_result_t> Factory>
    constexpr inline auto couple_like(Factory const& coupling) const {
      using factory_t = Factory;

      class body {
      public:
        using reproduction_context_t = internal_reproduction_context_t;
        using coupling_t = factory_result_t<factory_t, reproduction_context_t>;
        using copuling_result_t = std::invoke_result_t<
            coupling_t,
            std::add_lvalue_reference_t<std::add_const_t<selection_result_t>>>;

      public:
        constexpr inline explicit body(factory_t const& coupling)
            : coupling_{coupling} {
        }

        inline auto coupling(reproduction_context_t& context) const {
          return coupling_(context);
        }

      private:
        factory_t coupling_;
      };

      return this->template next<>(body{coupling});
    }
  };

  template<typename Built>
  struct select_ptype : public details::ptype_base<Built, select_ptype> {
    using population_t = typename Built::population_t;

    template<selection<population_t> Selection>
    constexpr inline auto select_using(Selection const& selection) const {
      class body {
      public:
        using selection_t = Selection;
        using selection_result_t = std::invoke_result_t<
            selection_t,
            std::add_lvalue_reference_t<std::add_const_t<population_t>>>;

      public:
        constexpr inline explicit body(selection_t const& selection)
            : selection_{selection} {
        }

        inline auto const& selection() const noexcept {
          return selection_;
        }

      private:
        selection_t selection_;
      };

      return this->template next<>(body{selection});
    }
  };

  template<typename Built>
  struct scale_ptype : public details::ptype_base<Built, scale_ptype> {
    using population_context_t = typename Built::population_context_t;

    template<scaling_factory<population_context_t> Factory>
    constexpr inline auto scale_as(Factory const& scaling) const {
      using factory_t = Factory;
      using chromosome_t = typename Built::chromosome_t;
      using raw_fitness_t = typename Built::raw_fitness_t;
      using population_t = typename Built::population_t;

      class body {
      public:
        using scaling_t = factory_result_t<factory_t, population_context_t>;
        using is_global_scaling_t = can_scale_global<scaling_t, population_t>;
        using is_stable_scaling_t =
            typename scaling_traits<scaling_t>::is_stable_t;

      public:
        constexpr inline explicit body(factory_t const& scaling)
            : scaling_{scaling} {
        }

        inline auto scaling(population_context_t& context) const {
          return scaling_(context);
        }

      private:
        factory_t scaling_;
      };

      return this->template next<>(body{scaling});
    }
  };

  template<typename Built>
  struct reproduce_ptype : public details::ptype_base<Built, reproduce_ptype> {
    template<crossover<typename Built::chromosome_t> Crossover,
             mutation<typename Built::chromosome_t> Mutation>
    constexpr inline auto reproduce_using(Crossover const& crossover,
                                          Mutation const& mutation) const {
      using chromosome_t = typename Built::chromosome_t;

      class body {
      public:
        using crossover_t = Crossover;
        using mutation_t = Mutation;

      public:
        constexpr inline explicit body(crossover_t const& crossover,
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

      return this->template next<>(body{crossover, mutation});
    }
  };

  template<typename Built>
  struct statistics_ptype
      : public details::ptype_base<Built, statistics_ptype> {
    using internal_population_t = population<typename Built::chromosome_t,
                                             typename Built::raw_fitness_t,
                                             typename Built::scaled_fitness_t,
                                             typename Built::tags_t>;

    template<stat::model<internal_population_t>... Models>
    constexpr inline auto track_these(std::size_t depth) const {
      class body {
      public:
        using population_t = internal_population_t;
        using statistics_t = stat::statistics<population_t, Models...>;
        using population_context_t =
            population_context<population_t, statistics_t>;

      public:
        inline explicit body(std::size_t depth) noexcept
            : depth_{depth} {
        }

        inline auto depth() const noexcept {
          return depth_;
        }

      private:
        std::size_t depth_;
      };

      return this->template next<>(body{depth});
    }
  };

  template<typename Built>
  class scale_fitness_ptype
      : public details::ptype_base<Built, scale_fitness_ptype> {
  private:
    class body_base {};
    class body_base_none {
    public:
      using is_global_scaling_t = std::true_type;
      using is_stable_scaling_t = std::true_type;
    };

    template<fitness Scaled>
    constexpr inline auto scale_to() const {
      return scale_impl<Scaled, body_base>();
    }

    constexpr inline auto scale_none() const {
      return scale_impl<empty_fitness, body_base_none>();
    }

  private:
    template<fitness Scaled, typename Base>
    constexpr inline auto scale_impl() const {
      class body : public Base {
      public:
        using scaled_fitness_t = Scaled;
      };

      return this->template next<>(body{});
    }
  };

  template<typename Built>
  struct evaluate_ptype : public details::ptype_base<Built, evaluate_ptype> {
    template<evaluator<typename Built::chromosome_t> Evaluator>
    constexpr inline auto evaluate_against(Evaluator const& evaluator) const {
      using chromosome_t = typename Built::chromosome_t;

      class body {
      public:
        using evaluator_t = Evaluator;
        using raw_fitness_t = std::invoke_result_t<
            evaluator_t,
            std::add_lvalue_reference_t<std::add_const_t<chromosome_t>>>;

      public:
        constexpr inline explicit body(evaluator_t const& evaluator)
            : evaluator_{evaluator} {
        }

        inline auto const& evaluator() const noexcept {
          return evaluator_;
        }

      private:
        evaluator_t evaluator_;
      };

      return this->template next<>(body{evaluator});
    }
  };

  template<typename Built>
  struct init_ptype : public details::ptype_base<Built, init_ptype> {
    template<initializator Initializator>
    constexpr inline auto make_like(Initializator const& initializator) const {
      class body {
      public:
        using initializator_t = Initializator;
        using chromosome_t = std::invoke_result_t<initializator_t>;

      public:
        constexpr inline explicit body(initializator_t const& initializator)
            : initializator_{initializator} {
        }

        inline auto const& initializator() const noexcept {
          return initializator_;
        }

      private:
        initializator_t initializator_;
      };

      return this->template next<>(body{initializator});
    }
  };

  template<typename Built>
  struct size_ptype : public details::ptype_base<Built, init_ptype> {
    constexpr inline auto limit_to(size_t size) const {
      class body {
      public:
        constexpr inline explicit body(size_t size)
            : size_{size} {
        }

        inline auto const& population_size() const noexcept {
          return size_;
        }

      private:
        std::size_t size_;
      };

      return this->template next<>(body{size});
    }
  };

  template<typename Built>
  struct root_ptype : public details::ptype_base<Built, root_ptype> {
    constexpr inline auto begin() const {
      return this->template next<>(details::empty_section{});
    }
  };

  template<template<typename> class Root, typename Entries>
  struct builder : details::builder_node<Entries,
                                         plist<Root>,
                                         plist<>,
                                         details::empty_section> {
    // using entries_t = Entries;
  };

} // namespace config
} // namespace gal
