///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MoAoOrder.C
//
// See include/tdat/MoAoOrder.H for why this exists.
///////////////////////////////////////////////////////////////////////////

#include "tdat/MoAoOrder.H"

#include <algorithm>
using std::stable_sort;

#include <memory>
#include <set>
#include <sstream>

#include "tdat/BasisFlatten.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/MoBasisChange.H"
#include "tdat/PropTable.H"
#include "tdat/PropString.H"

#include "dsm/ICalcUtils.H"
#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
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

namespace {

// CODEBASIS ("<centre> <L> exp coef ..." records joined by "|", written by
// gaussian-16.codebasis) as NumericalBasis text for ICalcUtils::importConfig.
// The first centre of each element supplies its basis.
string codeBasisText(const string& stored, const vector<string>& atomSymbols,
                     bool cartesian) {
  std::ostringstream os;
  os << "NumericalBasis\nbasis \"ao basis\" "
     << (cartesian ? "cartesian" : "spherical") << " print\n";
  std::set<string> done;
  string curElement;
  bool skipping = false;
  size_t pos = 0;
  while (pos <= stored.size()) {
    size_t bar = stored.find('|', pos);
    if (bar == string::npos) bar = stored.size();
    std::istringstream rec(stored.substr(pos, bar - pos));
    pos = bar + 1;
    size_t centre = 0;
    string shell;
    if (!(rec >> centre >> shell) || centre < 1 || centre > atomSymbols.size())
      return "";
    const string& el = atomSymbols[centre - 1];
    if (el != curElement) {
      skipping = done.count(el) > 0;
      done.insert(el);
      curElement = el;
    }
    if (skipping) continue;
    string e, c;
    os << el << " " << shell << "\n";
    while (rec >> e >> c) os << "  " << e << " " << c << "\n";
  }
  os << "END\nEndNumericalBasis\n";
  return os.str();
}

}  // namespace

namespace MoAoOrder {

PropTable* projectFromCodeBasis(PropTable *mo, const string& stored,
                                const vector<string>& atomSymbols,
                                TGBSConfig *gbsConfig, const JCode *code) {
  if (mo == 0 || gbsConfig == 0 || code == 0 || stored.empty()) return mo;
  const bool cartesian =
      (gbsConfig->coordsys() == TGaussianBasisSet::Cartesian);
  const string text = codeBasisText(stored, atomSymbols, cartesian);
  if (text.empty()) return mo;
  std::istringstream is(text);
  std::unique_ptr<TGBSConfig> codeCfg(ICalcUtils::importConfig(is));
  if (codeCfg == 0) return mo;

  std::unique_ptr<TGBSAngFunc> af(code->getAngFunc(
      cartesian ? TGaussianBasisSet::Cartesian : TGaussianBasisSet::Spherical));
  if (af == 0) return mo;
  const int lenCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
  const int lenSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
  // Only within-atom overlaps are used, so the centres need not be real.
  const vector<double> coords(3 * atomSymbols.size(), 0.0);
  vector<EspBasisFunction> from, to;
  if (!BasisFlatten::flatten(atomSymbols, coords, codeCfg.get(), code,
                             af.get(), af->maxShells(),
                             cartesian ? lenCart : lenSph, from) ||
      !BasisFlatten::flatten(atomSymbols, coords, gbsConfig, code, af.get(),
                             af->maxShells(), cartesian ? lenCart : lenSph, to))
    return mo;
  return MoBasisChange::project(mo, from, to);
}

static PropTable* reorderOnly(PropTable *moTable, IPropCalculation *calc,
                            const vector<string>& atomSymbols,
                            TGBSConfig *gbsConfig) {
  if (moTable == 0 || calc == 0 || gbsConfig == 0) return moTable;

  PropString *marker = (PropString*)calc->getProperty("MOAOORDER");
  if (marker == 0 || marker->value() != "angular-momentum") return moTable;

  // The recorded coordinate system first, the other only if its width
  // is not the table's: a spherical-only code (ECCE-QM, ORCA) runs a
  // basis recorded as Cartesian (Pople sets) with five d functions, and
  // giving up there left every column unpermuted.
  const bool recordedCartesian =
      (gbsConfig->coordsys() == TGaussianBasisSet::Cartesian);
  vector<Shell> shells;
  bool cartesian = recordedCartesian;
  for (int attempt = 0; attempt < 2; attempt++) {
    cartesian = (attempt == 0) ? recordedCartesian : !recordedCartesian;
    // Walk TGBSConfig exactly the way every consumer does (atom, then
    // getGBSList()'s insertion order, then contracted set, then column)
    // to get the NATIVE shell sequence and each shell's native column
    // offset.
    shells.clear();
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
    if (nativeCol == moTable->columns()) break;
    if (attempt == 1) return moTable;  // can't make sense of it
  }

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

PropTable* reorderToNative(PropTable *moTable, IPropCalculation *calc,
                            const vector<string>& atomSymbols,
                            TGBSConfig *gbsConfig) {
  PropTable *reordered = reorderOnly(moTable, calc, atomSymbols, gbsConfig);
  if (reordered == 0 || calc == 0 || gbsConfig == 0) return reordered;
  PropString *stored = (PropString*)calc->getProperty("CODEBASIS");
  ICalculation *escalc = dynamic_cast<ICalculation*>(calc);
  if (stored == 0 || escalc == 0) return reordered;
  PropTable *projected = projectFromCodeBasis(
      reordered, stored->value(), atomSymbols, gbsConfig, escalc->application());
  // The intermediate table is ours when the reorder made one.
  if (projected != reordered && reordered != moTable) delete reordered;
  return projected;
}

}  // namespace MoAoOrder
