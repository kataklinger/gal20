
#pragma once

#include "multiobjective.hpp"

namespace gal {
namespace project {

  // x = f(rank) / (1 - density) (nsga)
  template<typename RankTag>
  class scale {
  public:
    template<niched_individual<RankTag> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = f(rank) + g(density) (spea-ii)
  template<typename RankTag>
  class translate {
  public:
    template<niched_individual<RankTag> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = <rank, density> (nsga-ii, spea)
  template<typename RankTag>
  class merge {
  public:
    template<niched_individual<RankTag> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x = rank or x = density (pesa, pesa-ii, paes)
  template<typename RankTag>
  class truncate {
  public:
    template<niched_individual<RankTag> Individual>
    void operator()(Individual& individual) const {
    }
  };

  // x0 = rank, x1 = density, ... (rdga)
  template<typename RankTag>
  class alternate {
  public:
    template<niched_individual<RankTag> Individual>
    void operator()(Individual& individual) const {
    }
  };

} // namespace project
} // namespace gal
