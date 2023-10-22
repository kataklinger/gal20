
#pragma once

namespace gal {
namespace niche {

  class share {}; // fitness sharing (nsga)
  class distance {}; // crowding distance (nsga-ii)
  class cluster {}; // average linkage (spea)
  class neighbor {}; // kth nearest neighbor (spea-ii)
  class hypergrid {}; // cell-sharing (pesa, pesa-ii, paes)
  class adaptive_hypergrid {}; // adaptive cell-sharing (rdga)

} // namespace niche
} // namespace gal
