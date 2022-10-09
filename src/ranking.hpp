
#pragma once

namespace gal {
namespace rank {

  class binary {}; // non-dominated vs dominated (pesa, pesa-ii, paes)
  class level {}; // pareto front level (nsga & nsga-ii)
  class strength {}; // number of dominated, first front (spea)
  class acc_strength {}; // number of dominated, all fronts (spea-ii)
  class acc_level {}; // accumulated pareto level (rdga)

} // namespace rank
} // namespace gal
