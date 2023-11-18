
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace project {

  namespace details {

    template<typename Individual, typename From>
    concept projectable_from =
        std::constructible_from<typename Individual::scaled_fitness_t, From> &&
        std::assignable_from<
            std::add_lvalue_reference_t<typename Individual::scaled_fitness_t>,
            From>;

  } // namespace details

  template<typename Individual, typename RankTag, typename From>
  concept projectable = niched_individual<Individual, RankTag> &&
                        details::projectable_from<Individual, From>;

  // x = f(rank) / (1 - density) (nsga)
  template<typename RankTag>
  class scale {
  public:
    template<projectable<RankTag, double> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = f(rank) + g(density) (spea-ii)
  template<typename RankTag>
  class translate {
  public:
    template<projectable<RankTag, double> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = <rank, density> (nsga-ii, spea)
  template<typename RankTag>
  class merge {
  public:
    template<projectable<RankTag, std::tuple<double, double>> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = rank or x = density (pesa, pesa-ii, paes)
  template<typename RankTag>
  class truncate {
  public:
    template<projectable<RankTag, double> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x0 = rank, x1 = density, ... (rdga)
  template<typename RankTag>
  class alternate {
  public:
    template<projectable<RankTag, double> Individual>
    void operator()(Individual& individual) const {
    }
  };

} // namespace project
} // namespace gal
