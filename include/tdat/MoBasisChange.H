/**
 * @file
 *
 * Re-express MO coefficients given in one basis in another basis that
 * spans the same space on every atom.  Gaussian re-contracts general
 * contractions that share primitives (cc-pVXZ), so its MO coefficients
 * refer to its own contracted functions, not to the ones ECCE wrote in
 * the input; projecting onto ECCE's functions atom by atom makes the
 * coefficients valid for ECCE's basis again (#921).
 */

#ifndef MOBASISCHANGE_HH
#define MOBASISCHANGE_HH

#include <vector>
using std::vector;

#include "tdat/EspField.H"

class PropTable;

namespace MoBasisChange {

  /**
   * Returns a NEW table with mo's columns re-expressed from the flattened
   * basis `from` in the flattened basis `to` (same length and function
   * order), or mo itself when nothing differs or the two do not span the
   * same space on some atom (residual above tolerance).
   */
  PropTable* project(PropTable *mo,
                     const vector<EspBasisFunction>& from,
                     const vector<EspBasisFunction>& to);
}

#endif
