
#pragma once

#include "operation.hpp"

namespace gal {
namespace couple {

  namespace details {

    template<typename Context, typename Population>
    concept with_scaling = requires(Context ctx) {
      { ctx.scaling() } -> scaling<Population>;
    };

    template<typename Context, typename Population>
    concept without_scaling = !with_scaling<Context, Population>;

    template<typename Population, without_scaling<Population> Context>
    auto reproduce(Context const& context) {
    }

    template<typename Population, with_scaling<Population> Context>
    auto reproduce(Context const& context) {
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
