
#pragma once

namespace gal {
namespace prune {

  class none {}; // none (nsga)
  class global_bottom {}; // top by assigned fitness (nsga-ii, spea-ii)
  class local_random {}; // remove random from group (pesa, pesa-ii, paes)
  class local_edge {}; // remove non-centroids (spea)

} // namespace prune
} // namespace gal
