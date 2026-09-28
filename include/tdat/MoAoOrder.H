/**
 * @file
 *
 * A code's own output can print an atom's basis functions in whatever
 * shell order its own internal library uses, which is not always
 * angular-momentum sorted (ORCA's def2-* metals interleave a
 * polarization shell among the d shells). A parser that can only see
 * the code's stdout, not ECCE's own TGBSConfig, cannot always recover
 * ECCE's real per-atom column order -- but it CAN always produce one
 * fixed, well-defined order of its own: grouped by atom, then stably
 * sorted by angular momentum (see scripts/parsers/orca.mo's
 * canonicalRowOrder()). That parser marks the calc with MOAOORDER so
 * this side knows which convention MO/MOBETA's columns are in.
 *
 * reorderToNative() undoes that: it walks TGBSConfig exactly the way
 * every basis-function consumer does (BasisFlatten::flatten,
 * ComputeMoCmd, MoCoeffs, MoDiagramPanel, MullikenPanel) to get the
 * NATIVE per-atom shell sequence, works out what the canonical
 * (parser-side) sequence would be from that same walk (a stable sort
 * by l, same rule the parser used), and permutes MO/MOBETA's columns
 * from canonical back to native -- so every consumer keeps reading
 * TGBSConfig's own order and never has to know this happened.
 *
 * A calc with no MOAOORDER (every calc parsed before this existed, and
 * every other code) is untouched: this must stay a no-op for them.
 */

#ifndef MOAOORDER_HH
#define MOAOORDER_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

class PropTable;
class IPropCalculation;
class TGBSConfig;

namespace MoAoOrder {

// Returns moTable's columns (basis functions) reordered from the
// parser's canonical (atom, angular-momentum) order to TGBSConfig's own
// enumeration order, IF calc carries MOAOORDER=="angular-momentum" --
// otherwise (absent, any other value, or a width mismatch it can't make
// sense of) returns moTable UNCHANGED. Never mutates moTable itself
// (which may be a cached property shared across many calls -- the
// reorder must not be applied to it more than once): when a reorder is
// actually needed this returns a NEW PropTable, which the caller owns
// and must delete once done, e.g.:
//
//   PropTable *moCoefs = (PropTable*) calc->getProperty("MO");
//   PropTable *reordered = MoAoOrder::reorderToNative(moCoefs, calc, ...);
//   bool owned = (reordered != moCoefs);
//   moCoefs = reordered;
//   ... use moCoefs ...
//   if (owned) delete moCoefs;
//
// Only the shell TYPES (s/p/d/f/...) and TGBSConfig's own insertion
// order matter here, not a code's angular-function table, so this
// needs no JCode/TGBSAngFunc.
PropTable* reorderToNative(PropTable *moTable, IPropCalculation *calc,
                            const vector<string>& atomSymbols,
                            TGBSConfig *gbsConfig);

}  // namespace MoAoOrder

#endif  // MOAOORDER_HH
