
#pragma once

namespace gal {
namespace elite {

  class none {}; // no elitism, remove previous generation (nsga)
  class strict {}; // only non-dominated (pesa, pesa-ii, paes)
  class soft {}; // accept dominated to fill (nsga, spea-ii)
  class open {}; // accept all new individuals (spea)

} // namespace elite
} // namespace gal
