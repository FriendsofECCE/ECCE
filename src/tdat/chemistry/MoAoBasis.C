#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

#include "tdat/MoAoBasis.H"
#include "tdat/MoAoOrder.H"
#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/PropTable.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/ICalcUtils.H"
#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"


//  A shell of angular momentum l has (l+1)(l+2)/2 Cartesian functions
//  or 2l+1 spherical ones, in whatever order the code stores them.
static bool countOne(TGBSConfig *config, const vector<string>& symbols,
                     bool cartesian, vector<int>& counts,
                     vector<int>& shellOf, vector<int>& shellSeq,
                     vector<int>& component)
{
  counts.clear(); shellOf.clear(); shellSeq.clear(); component.clear();

  bool ok = true;
  for (size_t a = 0; a < symbols.size(); a++) {
    const string& symbol = symbols[a];
    int here = 0;
    std::map<int,int> seen;             // l -> shells so far on this atom

    vector<const TGaussianBasisSet*> list = config->getGBSList(symbol);
    for (size_t g = 0; g < list.size(); g++) {
      const TGaussianBasisSet *gbs = list[g];
      if (gbs == 0) continue;
      const int sets = gbs->num_contracted_sets(symbol.c_str());
      for (int ics = 0; ics < sets; ics++) {
        vector<TGaussianBasisSet::AngularMomentum> types =
            gbs->func_types(symbol.c_str(), ics);
        for (size_t t = 0; t < types.size(); t++) {
          const int l = (int)types[t];
          const int inShell = cartesian ? ((l+1)*(l+2))/2 : (2*l + 1);
          here += inShell;
          const int seq = ++seen[l];
          for (int f = 0; f < inShell; f++) {
            shellOf.push_back(l);
            shellSeq.push_back(seq);
            component.push_back(f);
          }
        }
      }
    }
    if (here == 0) ok = false;
    counts.push_back(here);
  }

  return ok && !counts.empty();
}


bool MoAoBasis::fragmentGeometry(Fragment *frag, vector<string>& symbols,
                                 vector<double>& coords)
{
  symbols.clear();
  coords.clear();
  if (frag == 0) return false;
  vector<TAtm*> *atoms = frag->atoms();
  double *xyz = frag->coordinates();
  const unsigned long natoms = frag->numAtoms();
  const bool ok = (atoms != 0 && xyz != 0 && natoms != 0 &&
                   atoms->size() == natoms);
  if (ok) {
    for (unsigned long a = 0; a < natoms; a++) {
      symbols.push_back((*atoms)[a]->atomicSymbol());
      for (int k = 0; k < 3; k++) coords.push_back(xyz[a*3+k]);
    }
  }
  delete atoms;
  return ok;
}


bool MoAoBasis::functionCounts(IPropCalculation *expt,
                               const vector<string>& symbols, int width,
                               vector<int>& counts, vector<int>& shellOf,
                               bool *semiempirical, vector<int> *seqOut,
                               vector<int> *componentOut)
{
  counts.clear();
  shellOf.clear();
  if (seqOut) seqOut->clear();
  if (componentOut) componentOut->clear();
  if (semiempirical != 0) *semiempirical = false;

  ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
  if (escalc == 0 || symbols.empty()) return false;

  TGBSConfig *config = escalc->gbsConfig();

  //  A semiempirical code writes no basis set; rebuild one from the
  //  Slater exponents it did report.  Its coefficients are already in an
  //  orthonormal basis (S=I), so a caller must not push them through a
  //  real Slater overlap (#175).
  if (config == 0 || config->empty()) {
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt);
    if (slater != 0) {
      delete config;
      config = slater;
      if (semiempirical != 0) *semiempirical = true;
    }
  }
  if (config == 0 || config->empty()) { delete config; return false; }

  const JCode *cap = escalc->application();
  TGBSAngFunc *angfunc = (cap == 0) ? 0 : cap->getAngFunc(config->coordsys());
  const bool recordedCartesian =
      (angfunc != 0 && angfunc->basisType() == TGBSAngFunc::Cartesian);
  delete angfunc;

  //  The recorded system first, the other only if it does not
  //  reproduce the coefficient table's width: a spherical-only code ran
  //  a nominally Cartesian Pople basis with five d functions, not six.
  //  A mapping that is off by one atom gives a plausible population
  //  instead of an error, hence the width check.
  bool ok = false;
  const bool tries[2] = { recordedCartesian, !recordedCartesian };
  for (int t = 0; t < 2 && !ok; t++) {
    vector<int> c, s, q, k;
    if (!countOne(config, symbols, tries[t], c, s, q, k)) continue;
    int total = 0;
    for (size_t i = 0; i < c.size(); i++) total += c[i];
    if (width > 0 && total != width) continue;
    counts.swap(c);
    shellOf.swap(s);
    if (seqOut) seqOut->swap(q);
    if (componentOut) componentOut->swap(k);
    ok = true;
  }

  delete config;
  return ok;
}


bool MoAoBasis::overlapMatrix(IPropCalculation *expt,
                              const vector<string>& storedElements,
                              const vector<double>& storedCoords, int width,
                              vector<int>& perAtom, vector<double>& Sflat,
                              std::function<void(double)> onRow,
                              vector<EspBasisFunction> *basisOut)
{
  perAtom.clear();
  Sflat.clear();

  ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
  const unsigned long natoms = storedElements.size();
  if (escalc == 0 || natoms == 0 || storedCoords.size() != natoms*3)
    return false;

  TGBSConfig *config = escalc->gbsConfig();
  if (config == 0 || config->empty()) {
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt);
    if (slater != 0) { delete config; config = slater; }
  }
  if (config == 0 || config->empty()) { delete config; return false; }

  const JCode *code = escalc->application();
  if (code == 0) { delete config; return false; }

  //  The recorded system first -- same two-try rule as functionCounts().
  vector<EspBasisFunction> basis;
  {
    int lengthShellCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
    int lengthShellSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
    const TGaussianBasisSet::CoordinateSystem recorded = config->coordsys();
    const TGaussianBasisSet::CoordinateSystem other =
        (recorded == TGaussianBasisSet::Spherical)
          ? TGaussianBasisSet::Cartesian : TGaussianBasisSet::Spherical;
    const TGaussianBasisSet::CoordinateSystem tries[2] = { recorded, other };
    for (int t = 0; t < 2 && basis.empty(); t++) {
      TGBSAngFunc *candidate = code->getAngFunc(tries[t]);
      if (candidate == 0) continue;
      const bool sph = (tries[t] == TGaussianBasisSet::Spherical);
      vector<EspBasisFunction> trial;
      if (BasisFlatten::flatten(storedElements, storedCoords, config, code,
                                candidate, candidate->maxShells(),
                                sph ? lengthShellSph : lengthShellCart,
                                trial) &&
          (int)trial.size() == width) {
        basis.swap(trial);
      }
      delete candidate;
    }
  }
  delete config;
  if (basis.empty()) return false;
  for (size_t i = 0; i < basis.size(); i++) if (basis[i].empty()) return false;

  {
    size_t i = 0;
    while (i < basis.size()) {
      size_t j = i;
      while (j < basis.size() &&
             fabs(basis[j].center[0]-basis[i].center[0]) < 1.0e-9 &&
             fabs(basis[j].center[1]-basis[i].center[1]) < 1.0e-9 &&
             fabs(basis[j].center[2]-basis[i].center[2]) < 1.0e-9) j++;
      perAtom.push_back((int)(j - i));
      i = j;
    }
  }
  if (perAtom.size() != natoms) { perAtom.clear(); return false; }

  const size_t nbasis = basis.size();
  Sflat.assign(nbasis*nbasis, 0.0);
  for (size_t i = 0; i < nbasis; i++) {
    for (size_t j = 0; j < nbasis; j++)
      Sflat[i*nbasis + j] = EspField::overlapOf(basis[i], basis[j]);
    if (onRow) onRow((i+1.0)/nbasis);
  }

  if (basisOut != 0) basisOut->swap(basis);
  return true;
}


static string monomial(int x, int y, int z)
{
  string s;
  s.append(x, 'x'); s.append(y, 'y'); s.append(z, 'z');
  return s;
}

string MoAoBasis::componentName(const EspBasisFunction& f, int l,
                                int indexInShell)
{
  if (l == 0) return "s";
  const size_t n = f.angularCoef.size();
  if (n == 0) return string();

  //  Dominant term: the one with the largest coefficient.
  size_t big = 0;
  for (size_t k = 1; k < n; k++)
    if (fabs(f.angularCoef[k]) > fabs(f.angularCoef[big]) + 1.0e-12) big = k;

  if (n == 1 || l == 1)
    return monomial(f.powerX[big], f.powerY[big], f.powerZ[big]);

  if (l == 2) {
    //  Spherical d: xz, yz, xy are one term; z2 is 2zz-xx-yy; x2-y2 is
    //  xx-yy.  The last two are told apart by whether a zz term exists.
    bool hasZZ = false;
    for (size_t k = 0; k < n; k++)
      if (f.powerZ[k] == 2) hasZZ = true;
    return hasZZ ? "z2" : "x2-y2";
  }

  //  f and higher, spherical: the code's own index in the shell.
  char buf[16];
  snprintf(buf, sizeof buf, "%c#%d", "spdfghik"[std::min(l, 7)],
           indexInShell + 1);
  return buf;
}


bool MoAoBasis::build(IPropCalculation *calc, const vector<string>& symbols,
                      const vector<double>& coordsAng, int width,
                      std::function<void(double)> onRow)
{
  elements = symbols;
  perAtom.clear(); functions.clear(); overlap.clear();
  orthonormal = false;
  complaint.clear();

  vector<int> counts, shellOf, seq, comp;
  bool semi = false;
  if (!functionCounts(calc, symbols, width, counts, shellOf, &semi, &seq,
                      &comp)) {
    complaint = "no basis set, or one that does not match the "
                "orbital coefficients";
    return false;
  }

  vector<EspBasisFunction> basis;
  if (semi) {
    orthonormal = true;
  } else if (!overlapMatrix(calc, symbols, coordsAng, width, perAtom,
                            overlap, onRow, &basis) ||
             perAtom != counts) {
    perAtom.clear(); overlap.clear();
    complaint = "the overlap matrix of the stored basis could not be "
                "built";
    return false;
  }
  if (perAtom.empty()) perAtom = counts;

  functions.resize(shellOf.size());
  size_t mu = 0;
  for (size_t a = 0; a < counts.size(); a++) {
    for (int k = 0; k < counts[a]; k++, mu++) {
      Function& f = functions[mu];
      f.atom = (int)a;
      f.l = shellOf[mu];
      f.shellSeq = seq[mu];
      f.component = basis.empty() ? string(1, "spdfghik"[std::min(f.l, 7)])
                                  : componentName(basis[mu], f.l, comp[mu]);
    }
  }
  return true;
}


const char* MoAoBasis::methodName(Method m)
{
  return m == MULLIKEN ? "Mulliken" : "c^2";
}


bool MoAoBasis::analyse(const vector<double>& c, Method method,
                        Composition& out) const
{
  const size_t nbas = functions.size();
  out = Composition();
  out.method = method;
  if (nbas == 0 || c.size() != nbas) return false;

  const bool useS = (method == MULLIKEN && !orthonormal &&
                     overlap.size() == nbas*nbas);
  vector<double> w(nbas, 0.0);
  double total = 0.0;
  for (size_t mu = 0; mu < nbas; mu++) {
    if (useS) {
      double sc = 0.0;
      const double *row = &overlap[mu*nbas];
      for (size_t nu = 0; nu < nbas; nu++) sc += row[nu]*c[nu];
      w[mu] = c[mu]*sc;
    } else {
      w[mu] = c[mu]*c[mu];
    }
    total += w[mu];
  }
  out.norm = total;
  if (!(total > 1.0e-12)) return false;

  out.perFunction.resize(nbas);
  for (size_t mu = 0; mu < nbas; mu++) out.perFunction[mu] = w[mu]/total;

  //  Groups in order of first appearance, so a listing reads
  //  atom by atom, s before p before d.
  std::map< std::pair<int,int>, size_t > typeAt;
  std::map< std::vector<int>, size_t > shellAt;
  const int natoms = (int)elements.size();
  out.atoms.resize(natoms);
  for (int a = 0; a < natoms; a++) {
    char buf[32];
    snprintf(buf, sizeof buf, "%s%d", elements[a].c_str(), a + 1);
    out.atoms[a].atom = a;
    out.atoms[a].label = buf;
  }
  static const char letter[] = "spdfghik";
  for (size_t mu = 0; mu < nbas; mu++) {
    const Function& f = functions[mu];
    const double s = out.perFunction[mu];
    out.atoms[f.atom].share += s;

    const std::pair<int,int> tk(f.atom, f.l);
    if (typeAt.find(tk) == typeAt.end()) {
      Share t;
      t.atom = f.atom; t.l = f.l; t.shellSeq = 0;
      t.label = out.atoms[f.atom].label + " " + letter[std::min(f.l, 7)];
      typeAt[tk] = out.types.size();
      out.types.push_back(t);
    }
    out.types[typeAt[tk]].share += s;

    std::vector<int> sk(3);
    sk[0] = f.atom; sk[1] = f.l; sk[2] = f.shellSeq;
    if (shellAt.find(sk) == shellAt.end()) {
      Share t;
      t.atom = f.atom; t.l = f.l; t.shellSeq = f.shellSeq;
      char buf[16];
      snprintf(buf, sizeof buf, "%d%c", f.shellSeq, letter[std::min(f.l, 7)]);
      t.label = out.atoms[f.atom].label + " " + buf;
      shellAt[sk] = out.shells.size();
      out.shells.push_back(t);
    }
    out.shells[shellAt[sk]].share += s;
  }
  return true;
}


bool MoAoBasis::coefficients(IPropCalculation *calc,
                             const vector<string>& symbols, bool beta,
                             vector< vector<double> >& rows)
{
  rows.clear();
  PropTable *moCoefs =
      (PropTable*) calc->getProperty(beta ? "MOBETA" : "MO");
  if (moCoefs == 0) return false;

  //  MOAOORDER-marked calcs (ORCA) keep the parser's column order, not
  //  TGBSConfig's -- see MoAoOrder.H.  The reordered copy is ours; the
  //  cached property must never be touched.
  std::unique_ptr<PropTable> owned;
  ICalculation *escalc = dynamic_cast<ICalculation*>(calc);
  std::unique_ptr<TGBSConfig> config(escalc != 0 ? escalc->gbsConfig() : 0);
  if (config != 0) {
    PropTable *reordered =
        MoAoOrder::reorderToNative(moCoefs, calc, symbols, config.get());
    if (reordered != moCoefs) { owned.reset(reordered); moCoefs = reordered; }
  }
  if (moCoefs->rows() <= 0 || moCoefs->columns() <= 0) return false;

  rows.assign(moCoefs->rows(), vector<double>(moCoefs->columns()));
  for (int m = 0; m < moCoefs->rows(); m++)
    for (int mu = 0; mu < moCoefs->columns(); mu++)
      rows[m][mu] = moCoefs->value(m, mu);
  return true;
}
