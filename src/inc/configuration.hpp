
#pragma once

#include "context.hpp"
#include "observing.hpp"
#include "statistics.hpp"

namespace gal {
namespace config {

  // clang-format off

  template<typename Section>
  concept section =
      !std::is_final_v<Section> && std::copy_constructible<Section> &&
      std::move_constructible<Section>;

  // clang-format on

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

    class empty_ptype {
      template<typename Sink>
      inline constexpr explicit empty_ptype(Sink const* /*unused*/) noexcept {
      }
    };

    template<section... Sections>
    class section_node;

    template<>
    class section_node<> {};

    template<section Section>
    class section_node<Section> : public Section, public section_node<> {
    public:
      using section_t = Section;

    public:
      inline constexpr section_node(section_t const& section,
                                    section_node<> const& /*unused*/)
          : section_t{section} {
      }
    };

    template<section... Sections>
    class section_node<details::empty_section, Sections...>
        : public section_node<Sections...> {
    public:
      using section_t = details::empty_section;
      using base_t = section_node<Sections...>;

      inline constexpr section_node() noexcept {
      }

      inline constexpr section_node(section_t const& /*unused*/,
                                    base_t const& /*unused*/) noexcept {
      }
    };

    template<section Section, section... Sections>
    class section_node<Section, Sections...>
        : public Section, public section_node<Sections...> {
    public:
      using section_t = Section;
      using base_t = section_node<Sections...>;

    public:
      inline constexpr section_node(section_t const& current,
                                    base_t const& base)
          : section_t{current}
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
    inline constexpr auto plist_contain_v =
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
    inline constexpr auto is_ptype_satisfied_v =
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
    class ptype_node<Built, Used, plist<>> {
    public:
      inline constexpr explicit ptype_node(Built const* /*unused*/) noexcept {
      }
    };

    template<typename Built,
             typename Used,
             template<typename>
             class Interface,
             template<typename>
             class... Interfaces>
    class ptype_node<Built, Used, plist<Interface, Interfaces...>>
        : public ptype_inherit_t<Used, Interface<Built>>,
          public ptype_node<Built, Used, plist<Interfaces...>> {
    public:
      inline constexpr explicit ptype_node(Built const* current)
          : ptype_inherit_t<Used, Interface<Built>>{current}
          , ptype_node<Built, Used, plist<Interfaces...>>{current} {
      }
    };

    template<typename Entries,
             typename Available,
             typename Used,
             section... Sections>
    class builder_node;

    template<typename Entries,
             typename Available,
             typename Used,
             typename Section,
             typename... Sections>
    class built : public section_node<Section, Sections...> {
    public:
      using entries_t = Entries;
      using section_t = Section;

      using previous_t = section_node<Sections...>;
      using base_t = section_node<Section, Sections...>;

    public:
      template<
          typename = std::enable_if_t<std::is_same_v<section_t, empty_section>>>
      inline constexpr built() noexcept {
      }

      inline constexpr built(section_t const& current,
                             previous_t const& previous)
          : base_t{current, previous} {
      }

      template<template<typename> class This,
               typename Unlocked,
               section Appended>
      using builder_t =
          builder_node<entries_t,
                       plist_remove_t<This, plist_merge_t<Unlocked, Available>>,
                       plist_add_t<This, Used>,
                       Appended,
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
        : public built<Entries, Available, Used, Section, Rest...>,
          public ptype_node<built<Entries, Available, Used, Section, Rest...>,
                            Used,
                            Available> {
    public:
      using built_base_t = built<Entries, Available, Used, Section, Rest...>;
      using ptype_base_t =
          ptype_node<built<Entries, Available, Used, Section, Rest...>,
                     Used,
                     Available>;
      using current_section_t = typename built_base_t::section_t;
      using previous_section_t = typename built_base_t::previous_t;

    public:
      template<typename = std::enable_if_t<
                   std::is_same_v<current_section_t, empty_section>>>
      inline constexpr builder_node() noexcept
          : ptype_base_t{this} {
      }

      inline constexpr builder_node(current_section_t const& current,
                                    previous_section_t const& previous)
          : built_base_t{current, previous}
          , ptype_base_t{this} {
      }

      inline constexpr auto end() const {
        return static_cast<built_base_t>(*this);
      }

      template<template<typename...> class Algorithm, typename... Args>
      inline constexpr auto build(Args&&... args) const {
        return Algorithm{static_cast<built_base_t>(*this),
                         std::forward<Args>(args)...};
      }
    };

    template<typename Built>
    using get_entry_map = typename Built::entries_t::type;

    template<typename Built,
             template<typename>
             class Current,
             section Appended,
             typename Unlocked>
    using next_builder_t =
        typename Built::template builder_t<Current, Unlocked, Appended>;

    template<typename Built, template<typename> class Derived>
    class ptype_base {
    private:
      using entry_map_t = entry_map_match<get_entry_map<Built>, Derived, Built>;

    public:
      using required_t = typename entry_map_t::required_t;

    public:
      inline constexpr explicit ptype_base(Built const* current)
          : current_{current} {
      }

    protected:
      template<section Appended>
      inline constexpr auto next(Appended&& section) const {
        return next_builder_t<Built,
                              Derived,
                              Appended,
                              typename entry_map_t::unlocked_t>{
            std::forward<Appended>(section), *current_};
      }

    private:
      Built const* current_;
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

  template<typename Tags>
  struct tags_body {
    using tags_t = Tags;
  };

  template<typename Built>
  struct tags_ptype : public details::ptype_base<Built, tags_ptype> {
    inline constexpr explicit tags_ptype(Built const* current)
        : details::ptype_base<Built, tags_ptype>{current} {
    }

    template<typename... Tags>
    inline constexpr auto tag() const {
      return this->next(tags_body<std::tuple<Tags...>>{});
    }

    inline constexpr auto tag() const {
      return this->next(tags_body<empty_tags>{});
    }
  };

  template<typename Events, typename... Observers>
  class observe_body {
  public:
    using events_t = Events;
    using observers_t =
        observer_pack<events_t, std::remove_cvref_t<Observers>...>;

  public:
    inline explicit constexpr observe_body(events_t events,
                                           Observers&&... observers)
        : observers_{events, std::move(observers)...} {
    }

    inline auto& observers() {
      return observers_;
    }

  private:
    observers_t observers_;
  };

  template<typename Built>
  class observe_ptype : public details::ptype_base<Built, observe_ptype> {
  public:
    inline constexpr explicit observe_ptype(Built const* current)
        : details::ptype_base<Built, observe_ptype>{current} {
    }

    template<typename... Events, gal::observer<Built, Events>... Observers>
    inline constexpr auto
        observe(gal::observe<Events, Observers>... observers) {
      return this->next(observe_body{gal::observer_tags<Events...>{},
                                     std::move(observers).observer()...});
    }
  };

  template<typename Criterion>
  class criterion_body {
  public:
    using criterion_t = Criterion;

  public:
    inline constexpr explicit criterion_body(criterion_t const& criterion)
        : criterion_{criterion} {
    }

    inline auto criterion() const {
      return criterion_;
    }

  private:
    criterion_t criterion_;
  };

  template<typename Built>
  struct criterion_ptype : public details::ptype_base<Built, criterion_ptype> {
    using population_t = typename Built::population_t;
    using history_t = stats::history<typename Built::statistics_t>;

    inline constexpr explicit criterion_ptype(Built const* current)
        : details::ptype_base<Built, criterion_ptype>{current} {
    }

    template<criterion<population_t, history_t> Criterion>
    inline constexpr auto stop(Criterion const& criterion) const {
      return this->next(criterion_body{criterion});
    }
  };

  template<typename Replacement>
  class replace_body {
  public:
    using replacement_t = Replacement;

  public:
    inline constexpr explicit replace_body(replacement_t const& replacement)
        : replacement_{replacement} {
    }

    inline auto const& replacement() const noexcept {
      return replacement_;
    }

  private:
    replacement_t replacement_;
  };

  template<typename Built>
  struct replace_ptype : public details::ptype_base<Built, replace_ptype> {
    using population_t = typename Built::population_t;
    using copuling_result_t = typename Built::copuling_result_t;

    inline constexpr explicit replace_ptype(Built const* current)
        : details::ptype_base<Built, replace_ptype>{current} {
    }

    template<replacement<population_t, copuling_result_t> Replacement>
    inline constexpr auto replace(Replacement const& replacement) const {
      return this->next(replace_body{replacement});
    }
  };

  template<typename Factory, typename Context, typename Input>
  class couple_body {
  public:
    using reproduction_context_t = Context;
    using coupling_t = factory_result_t<Factory, reproduction_context_t>;
    using copuling_result_t = std::invoke_result_t<
        coupling_t,
        std::add_lvalue_reference_t<std::add_const_t<Input>>>;

  public:
    inline constexpr explicit couple_body(Factory const& coupling)
        : coupling_{coupling} {
    }

    inline auto coupling(reproduction_context_t& context) const {
      return coupling_(context);
    }

  private:
    Factory coupling_;
  };

  template<typename Built>
  struct couple_ptype : public details::ptype_base<Built, couple_ptype> {
    using reproduction_context_t = details::build_reproduction_context_t<Built>;
    using selection_result_t = typename Built::selection_result_t;

    inline constexpr explicit couple_ptype(Built const* current)
        : details::ptype_base<Built, couple_ptype>{current} {
    }

    template<
        coupling_factory<reproduction_context_t, selection_result_t> Factory>
    inline constexpr auto couple(Factory const& coupling) const {
      return this->next(
          couple_body<Factory, reproduction_context_t, selection_result_t>{
              coupling});
    }
  };

  template<typename Selection, typename Population>
  class select_body {
  public:
    using selection_t = Selection;
    using selection_result_t =
        std::invoke_result_t<selection_t,
                             std::add_lvalue_reference_t<Population>>;

  public:
    inline constexpr explicit select_body(selection_t const& selection)
        : selection_{selection} {
    }

    inline auto const& selection() const noexcept {
      return selection_;
    }

  private:
    selection_t selection_;
  };

  template<typename Built>
  struct select_ptype : public details::ptype_base<Built, select_ptype> {
    using population_t = typename Built::population_t;

    inline constexpr explicit select_ptype(Built const* current)
        : details::ptype_base<Built, select_ptype>{current} {
    }

    template<selection<population_t> Selection>
    inline constexpr auto select(Selection const& selection) const {
      return this->next(select_body<Selection, population_t>{selection});
    }
  };

  template<typename Factory,
           typename Scaling,
           typename Context,
           typename Population>
  class scaling_body {
  public:
    using scaling_t = factory_result_t<Factory, Context>;
    using is_global_scaling_t = can_scale_global<scaling_t, Population>;
    using is_stable_scaling_t = typename scaling_traits<scaling_t>::is_stable_t;

  public:
    inline constexpr explicit scaling_body(Factory const& scaling)
        : scaling_{scaling} {
    }

    inline auto scaling(Context& context) const {
      return scaling_(context);
    }

  private:
    Factory scaling_;
  };

  template<typename Built>
  struct scale_ptype : public details::ptype_base<Built, scale_ptype> {
    using population_context_t = typename Built::population_context_t;

    inline constexpr explicit scale_ptype(Built const* current)
        : details::ptype_base<Built, scale_ptype>{current} {
    }

    template<scaling_factory<population_context_t> Factory>
    inline constexpr auto scale_as(Factory const& scaling) const {
      using population_t = typename Built::population_t;

      return this->next(
          scaling_body<Factory, population_context_t, population_t>{scaling});
    }
  };

  template<typename Crossover, typename Mutation>
  class reproduce_body {
  public:
    using crossover_t = Crossover;
    using mutation_t = Mutation;

  public:
    inline constexpr explicit reproduce_body(crossover_t const& crossover,
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

  template<typename Built>
  struct reproduce_ptype : public details::ptype_base<Built, reproduce_ptype> {
    inline constexpr explicit reproduce_ptype(Built const* current)
        : details::ptype_base<Built, reproduce_ptype>{current} {
    }

    template<crossover<typename Built::chromosome_t> Crossover,
             mutation<typename Built::chromosome_t> Mutation>
    inline constexpr auto reproduce(Crossover const& crossover,
                                    Mutation const& mutation) const {
      return this->next(reproduce_body{crossover, mutation});
    }
  };

  template<typename Population, typename... Models>
  class statistics_body {
  public:
    using population_t = Population;
    using statistics_t = stats::statistics<population_t, Models...>;
    using history_t = stats::history<statistics_t>;
    using population_context_t = population_context<population_t, statistics_t>;

  public:
    inline explicit statistics_body(std::size_t depth) noexcept
        : depth_{depth} {
    }

    inline auto statistics_depth() const noexcept {
      return depth_;
    }

  private:
    std::size_t depth_;
  };

  template<typename Built>
  struct statistics_ptype
      : public details::ptype_base<Built, statistics_ptype> {
    using population_t = population<typename Built::chromosome_t,
                                    typename Built::raw_fitness_t,
                                    typename Built::raw_comparator_t,
                                    typename Built::scaled_fitness_t,
                                    typename Built::scaled_comparator_t,
                                    typename Built::tags_t>;

    inline constexpr explicit statistics_ptype(Built const* current)
        : details::ptype_base<Built, statistics_ptype>{current} {
    }

    template<stats::model<population_t>... Models>
    inline constexpr auto track(std::size_t depth) const {
      return this->next(statistics_body<population_t, Models...>{depth});
    }
  };

  class scale_fitness_body_base {};
  class scale_fitness_body_base_none {
  public:
    using is_global_scaling_t = std::true_type;
    using is_stable_scaling_t = std::true_type;
  };

  template<fitness Scaled, typename Comparator, typename Base>
  class scale_fitness_body : public Base {
  public:
    using scaled_fitness_t = Scaled;
    using scaled_comparator_t = Comparator;

  public:
    inline constexpr explicit scale_fitness_body(
        scaled_comparator_t const& compare)
        : scaled_comparator_{compare} {
    }

    inline auto const& scaled_comparator() const noexcept {
      return scaled_comparator_;
    }

  private:
    scaled_comparator_t scaled_comparator_;
  };

  template<typename Built>
  class scale_fitness_ptype
      : public details::ptype_base<Built, scale_fitness_ptype> {
  public:
    inline constexpr explicit scale_fitness_ptype(Built const* current)
        : details::ptype_base<Built, scale_fitness_ptype>{current} {
    }

    template<fitness Scaled, comparator<Scaled> Comparator>
    inline constexpr auto scale(Comparator const& compare) const {
      return scale_impl<Scaled, Comparator, scale_fitness_body_base>(compare);
    }

    inline constexpr auto scale() const {
      return scale_impl<empty_fitness,
                        disabled_comparator,
                        scale_fitness_body_base_none>({});
    }

  private:
    template<typename Scaled, typename Comparator, typename Base>
    inline constexpr auto scale_impl(Comparator const& compare) const {
      return this->next(scale_fitness_body<Scaled, Comparator, Base>{compare});
    }
  };

  template<typename Evaluator, typename Chromosome, typename Comparator>
  class evaluate_body {
  public:
    using evaluator_t = Evaluator;
    using raw_fitness_t = get_evaluator_result_t<Chromosome, Evaluator>;
    using raw_comparator_t = Comparator;

  public:
    inline constexpr explicit evaluate_body(
        evaluator_t const& evaluator,
        raw_comparator_t const& raw_comparator)
        : evaluator_{evaluator}
        , raw_comparator_{raw_comparator} {
    }

    inline auto const& evaluator() const noexcept {
      return evaluator_;
    }

    inline auto const& raw_comparator() const noexcept {
      return raw_comparator_;
    }

  private:
    evaluator_t evaluator_;
    raw_comparator_t raw_comparator_;
  };

  template<typename Built>
  struct evaluate_ptype : public details::ptype_base<Built, evaluate_ptype> {
    using chromosome_t = typename Built::chromosome_t;

    inline constexpr explicit evaluate_ptype(Built const* current)
        : details::ptype_base<Built, evaluate_ptype>{current} {
    }

    template<
        evaluator<chromosome_t> Evaluator,
        comparator<get_evaluator_result_t<chromosome_t, Evaluator>> Comparator>
    inline constexpr auto evaluate(Evaluator const& evaluator,
                                   Comparator const& compare) const {
      return this->next(evaluate_body<Evaluator, chromosome_t, Comparator>{
          evaluator, compare});
    }
  };

  template<typename Initializator>
  class init_body {
  public:
    using initializator_t = Initializator;
    using chromosome_t = std::invoke_result_t<initializator_t>;

  public:
    inline constexpr explicit init_body(initializator_t const& initializator)
        : initializator_{initializator} {
    }

    inline auto const& initializator() const noexcept {
      return initializator_;
    }

  private:
    initializator_t initializator_;
  };

  template<typename Built>
  struct init_ptype : public details::ptype_base<Built, init_ptype> {
    inline constexpr explicit init_ptype(Built const* current)
        : details::ptype_base<Built, init_ptype>{current} {
    }

    template<initializator Initializator>
    inline constexpr auto spawn(Initializator const& initializator) const {
      return this->next(init_body{initializator});
    }
  };

  class size_body {
  public:
    inline constexpr explicit size_body(size_t size)
        : size_{size} {
    }

    inline auto population_size() const noexcept {
      return size_;
    }

  private:
    std::size_t size_;
  };

  template<typename Built>
  struct size_ptype : public details::ptype_base<Built, size_ptype> {
    inline constexpr explicit size_ptype(Built const* current)
        : details::ptype_base<Built, size_ptype>{current} {
    }

    inline constexpr auto limit(size_t size) const {
      return this->next(size_body{size});
    }
  };

  template<typename Built>
  struct root_ptype : public details::ptype_base<Built, root_ptype> {
    inline constexpr explicit root_ptype(Built const* current)
        : details::ptype_base<Built, root_ptype>{current} {
    }

    inline constexpr auto begin() const {
      return this->next(details::empty_section{});
    }
  };

  template<typename Entries, template<typename> class Root>
  struct builder : details::builder_node<Entries,
                                         plist<Root>,
                                         plist<>,
                                         details::empty_section> {};

  template<typename Map, template<typename> class Root = root_ptype>
  inline constexpr auto for_map() {
    return builder<Map, Root>();
  }

} // namespace config
} // namespace gal
