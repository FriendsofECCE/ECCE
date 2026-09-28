///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MoAoOrder.C
//
// See include/tdat/MoAoOrder.H for why this exists.
///////////////////////////////////////////////////////////////////////////

#include "tdat/MoAoOrder.H"

#include <algorithm>
using std::stable_sort;

#include "tdat/PropTable.H"
#include "tdat/PropString.H"

#include "dsm/IPropCalculation.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSConfig.H"

namespace {

// (l+1)(l+2)/2 Cartesian functions in a shell, 2l+1 spherical ones --
// the same formula BasisFlatten/MoDiagramPanel/MoCoeffs each already
// hardcode at their own call sites.
int shellLength(int l, bool cartesian) {
  return cartesian ? ((l+1)*(l+2))/2 : (2*l + 1);
}

struct Shell {
  size_t atomIdx;
  int l;
  int nativeStart;     // column offset in TGBSConfig's own order
  int canonicalStart;  // column offset in the parser's canonical order
  int length;
  size_t nativeIndex;  // this shell's position in the native-order list,
                       // so sorting a COPY can report back to it
};

}  // namespace

namespace MoAoOrder {

PropTable* reorderToNative(PropTable *moTable, IPropCalculation *calc,
                            const vector<string>& atomSymbols,
                            TGBSConfig *gbsConfig) {
  if (moTable == 0 || calc == 0 || gbsConfig == 0) return moTable;

  PropString *marker = (PropString*)calc->getProperty("MOAOORDER");
  if (marker == 0 || marker->value() != "angular-momentum") return moTable;

  const bool cartesian =
      (gbsConfig->coordsys() == TGaussianBasisSet::Cartesian);

  // Walk TGBSConfig exactly the way every consumer does (atom, then
  // getGBSList()'s insertion order, then contracted set, then column)
  // to get the NATIVE shell sequence and each shell's native column
  // offset.
  vector<Shell> shells;
  int nativeCol = 0;
  for (size_t a = 0; a < atomSymbols.size(); a++) {
    vector<const TGaussianBasisSet*> gbslist =
        gbsConfig->getGBSList(atomSymbols[a]);
    for (size_t g = 0; g < gbslist.size(); g++) {
      const TGaussianBasisSet *gbs = gbslist[g];
      if (gbs == 0) continue;
      const size_t sets = gbs->num_contracted_sets(atomSymbols[a].c_str());
      for (size_t ics = 0; ics < sets; ics++) {
        vector<TGaussianBasisSet::AngularMomentum> types =
            gbs->func_types(atomSymbols[a].c_str(), ics);
        for (size_t t = 0; t < types.size(); t++) {
          Shell s;
          s.atomIdx = a;
          s.l = (int)types[t];
          s.nativeStart = nativeCol;
          s.canonicalStart = -1;  // filled in below
          s.length = shellLength(s.l, cartesian);
          s.nativeIndex = shells.size();
          shells.push_back(s);
          nativeCol += s.length;
        }
      }
    }
  }
  if (nativeCol != moTable->columns()) return moTable;  // can't make sense of it

  // The parser's canonical order: per atom, the SAME shells stably
  // sorted by l (see orca.mo's canonicalRowOrder -- this must match it).
  vector<Shell> canonical = shells;
  size_t start = 0;
  while (start < canonical.size()) {
    size_t end = start;
    while (end < canonical.size() &&
           canonical[end].atomIdx == canonical[start].atomIdx) end++;
    stable_sort(canonical.begin()+start, canonical.begin()+end,
                [](const Shell& x, const Shell& y) { return x.l < y.l; });
    start = end;
  }
  int canonicalCol = 0;
  for (size_t i = 0; i < canonical.size(); i++) {
    // Report the canonical offset back to the ORIGINAL (native-order)
    // shell -- canonical[i] is a reordered copy, not shells[i].
    shells[canonical[i].nativeIndex].canonicalStart = canonicalCol;
    canonicalCol += canonical[i].length;
  }
  if (canonicalCol != moTable->columns()) return moTable;

  const int rows = moTable->rows();
  const int cols = moTable->columns();
  vector<double> reordered(rows * (size_t)cols);
  for (int m = 0; m < rows; m++) {
    for (size_t i = 0; i < shells.size(); i++) {
      for (int off = 0; off < shells[i].length; off++) {
        reordered[m*(size_t)cols + shells[i].nativeStart + off] =
            moTable->value(m, shells[i].canonicalStart + off);
      }
    }
  }

  //  A NEW table, never a mutation of moTable -- moTable is typically a
  //  cached property that outlives a single call, and this reorder must
  //  only ever happen once, not once per caller per call.
  PropTable *result = new PropTable(*moTable);
  result->values(rows, cols, reordered);
  return result;
}

}  // namespace MoAoOrder
