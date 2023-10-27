
#pragma once

namespace gal {
namespace project {

  class scale {}; // x = f(rank) / (1 - density) (nsga)
  class translate {}; // x = f(rank) + g(density) (spea-ii)
  class merge {}; // x = <rank, density> (nsga-ii, spea)
  class truncate {}; // x = rank or x = density (pesa, pesa-ii, paes)
  class alternate {}; // x0 = rank, x1 = density, ... (rdga)

} // namespace project
} // namespace gal
