
#pragma once

namespace gal {
namespace niche {

  class sharing {}; // fitness sharing (nsga)
  class distancing {}; // crowding distance (nsga-ii)
  class clustering {}; // average linkage (spea)
  class neighboring {}; // kth nearest neighbor (spea-ii)
  class hypergrid {}; // cell-sharing (pesa, pesa-ii, paes)
  class adaptive_hypergrid {}; // adaptive cell-sharing (rdga)

} // namespace niche
} // namespace gal
