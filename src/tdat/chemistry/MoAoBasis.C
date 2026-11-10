#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <sstream>

#include "tdat/MoAoBasis.H"
#include "tdat/MoAoOrder.H"
#include "tdat/MoComposition.H"
#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/ResultsFromCalc.H"
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

  //  Rebuild the semiempirical basis from its reported exponents.
  //  MOPAC's parser back-transforms GRAPHF vectors into this AO basis.
  if (config == 0 || config->empty()) {
    bool orthonormal = false;
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt, &orthonormal);
    if (slater != 0) {
      delete config;
      config = slater;
      //  This flag identifies the Slater model, not the stored MO metric.
      if (semiempirical != 0) *semiempirical = orthonormal;
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


bool MoAoBasis::flatBasis(IPropCalculation *expt,
                          const vector<string>& storedElements,
                          const vector<double>& storedCoords, int width,
                          vector<EspBasisFunction>& basisOut)
{
  basisOut.clear();
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
  basisOut.swap(basis);
  return true;
}



//  GRAPHF's AO coefficients use its own frame, which can differ from
//  GEOMTRACE after a FORCE run. Its exact metric also avoids the error of
//  the Gaussian expansion used to render Slater functions.
static bool graphfBasisMetric(IPropCalculation *calc,
                           const vector<string>& symbols,
                           int width, vector<double>& coords,
                           vector<double>& overlap)
{
  ICalculation *escalc = dynamic_cast<ICalculation*>(calc);
  PropTable *slater = (PropTable*)calc->getProperty("SLATERBASIS");
  if (!escalc || !slater || slater->rows() != (int)symbols.size() ||
      slater->columns() != 4) return false;
  string name;
  for (const string& file : escalc->getAllFileNames()) {
    if (file.size() < 4 || file.substr(file.size()-4) != ".mgf") continue;
    if (!name.empty()) return false;
    name = file;
  }
  if (name.empty()) return false;
  std::unique_ptr<std::istream> in(escalc->getAnyFile(name));
  string line;
  int n = 0;
  if (!in || !std::getline(*in, line) ||
      !(std::istringstream(line) >> n) || n != (int)symbols.size()) return false;
  vector<double> xyz;
  for (int a = 0; a < n; a++) {
    int z;
    double x, y, zz;
    if (!std::getline(*in, line)) return false;
    std::istringstream row(line);
    if (!(row >> z >> x >> y >> zz) || z != (int)slater->value(a, 0) ||
        TAtm((short)z).atomicSymbol() != symbols[a] ||
        !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(zz)) return false;
    xyz.insert(xyz.end(), {x, y, zz});
  }
  for (int a = 0; a < n; a++) {
    if (!std::getline(*in, line)) return false;
    std::istringstream row(line);
    for (int k = 1; k <= 3; k++) {
      double zeta;
      if (!(row >> zeta) || !std::isfinite(zeta) ||
          fabs(zeta - slater->value(a, k)) > 1.0e-7) return false;
    }
  }
  int rows = 0, cols = 0;
  bool found = false;
  while (std::getline(*in, line)) {
    if (sscanf(line.c_str(), " INVERSE_MATRIX[%dx%d]=", &rows, &cols) == 2) {
      found = true;
      break;
    }
  }
  if (!found || rows != width || cols != width || width <= 0) return false;
  vector<double> triangle;
  while (std::getline(*in, line)) {
    if (line.find("Keywords:") != string::npos) break;
    std::replace(line.begin(), line.end(), 'D', 'E');
    std::replace(line.begin(), line.end(), 'd', 'e');
    std::istringstream row(line);
    double v;
    while (row >> v) {
      if (!std::isfinite(v)) return false;
      triangle.push_back(v);
    }
  }
  if (triangle.size() != (size_t)width*(width+1)/2) return false;
  vector<vector<double>> inverseHalf(width, vector<double>(width));
  size_t t = 0;
  for (int i = 0; i < width; i++)
    for (int j = 0; j <= i; j++)
      inverseHalf[i][j] = inverseHalf[j][i] = triangle[t++];
  vector<double> values;
  vector<vector<double>> vectors;
  if (!MoComposition::jacobiEigen(inverseHalf, values, vectors)) return false;
  overlap.assign((size_t)width*width, 0.0);
  //  The parser uses A = S^(-1/2), so S = V diag(lambda^-2) V^T.
  for (int k = 0; k < width; k++) {
    if (!std::isfinite(values[k]) || values[k] <= 1.0e-10) return false;
    const double s = 1.0/(values[k]*values[k]);
    for (int i = 0; i < width; i++)
      for (int j = 0; j < width; j++)
        overlap[i*width+j] += vectors[i][k]*s*vectors[j][k];
  }
  coords.swap(xyz);
  return true;
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

  vector<EspBasisFunction> basis;
  vector<double> graphCoords;
  const vector<double> *coords = &storedCoords;
  if (expt->getProperty("SLATERBASIS")) {
    if (!graphfBasisMetric(expt, storedElements, width, graphCoords, Sflat))
      return false;
    coords = &graphCoords;
  }
  if (!flatBasis(expt, storedElements, *coords, width, basis))
    return false;
  const unsigned long natoms = storedElements.size();

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
  if (Sflat.empty()) {
    Sflat.assign(nbasis*nbasis, 0.0);
    for (size_t i = 0; i < nbasis; i++) {
      for (size_t j = 0; j < nbasis; j++)
        Sflat[i*nbasis + j] = EspField::overlapOf(basis[i], basis[j]);
      if (onRow) onRow((i+1.0)/nbasis);
    }
  } else if (onRow) {
    onRow(1.0);
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
  if (!functionCounts(calc, symbols, width, counts, shellOf, nullptr, &seq,
                      &comp)) {
    complaint = "no basis set, or one that does not match the "
                "orbital coefficients";
    return false;
  }

  vector<EspBasisFunction> basis;
  if (!overlapMatrix(calc, symbols, coordsAng, width, perAtom,
                            overlap, onRow, &basis) ||
             perAtom != counts) {
    perAtom.clear(); overlap.clear();
    complaint = "the overlap matrix of the stored basis could not be "
                "built in the orbital frame (MOPAC requires its GRAPHF file)";
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


bool MoAoBasis::spinPopulations(const vector<SpinSet>& sets,
                                vector<double>& perAtom) const
{
  const size_t nbas = functions.size();
  perAtom.assign(elements.size(), 0.0);
  if (nbas == 0) return false;

  //  Orbitals with a net spin, stored mu-major so the inner loop over
  //  orbitals is contiguous.
  vector<double> net;
  vector<const vector<double>*> row;
  for (size_t s = 0; s < sets.size(); s++) {
    if (!sets[s].rows) continue;
    for (size_t i = 0; i < sets[s].net.size() && i < sets[s].rows->size(); i++) {
      if (sets[s].net[i] == 0.0) continue;
      if ((*sets[s].rows)[i].size() != nbas) return false;
      net.push_back(sets[s].net[i]);
      row.push_back(&(*sets[s].rows)[i]);
    }
  }
  const size_t norb = net.size();
  if (norb == 0) return true;

  vector<double> C(nbas*norb);
  for (size_t i = 0; i < norb; i++)
    for (size_t mu = 0; mu < nbas; mu++) C[mu*norb + i] = (*row[i])[mu];

  //  A[mu][i] = c_i[mu] (S c_i)[mu]; only nonzero S elements are visited,
  //  which is most of the saving for a large molecule.
  const bool useS = (!orthonormal && overlap.size() == nbas*nbas);
  vector<double> A(nbas*norb, 0.0);
  vector<double> norm(norb, 0.0);
  for (size_t mu = 0; mu < nbas; mu++) {
    double *a = &A[mu*norb];
    const double *cm = &C[mu*norb];
    if (!useS) {
      for (size_t i = 0; i < norb; i++) a[i] = cm[i]*cm[i];
    } else {
      const double *srow = &overlap[mu*nbas];
      for (size_t nu = 0; nu < nbas; nu++) {
        const double s = srow[nu];
        if (s == 0.0) continue;
        const double *cn = &C[nu*norb];
        for (size_t i = 0; i < norb; i++) a[i] += s*cm[i]*cn[i];
      }
    }
    for (size_t i = 0; i < norb; i++) norm[i] += a[i];
  }

  vector<double> scale(norb, 0.0);
  for (size_t i = 0; i < norb; i++)
    if (norm[i] > 1.0e-12) scale[i] = net[i]/norm[i];
  for (size_t mu = 0; mu < nbas; mu++) {
    const int atom = functions[mu].atom;
    if (atom < 0 || atom >= (int)perAtom.size()) continue;
    const double *a = &A[mu*norb];
    double q = 0.0;
    for (size_t i = 0; i < norb; i++) q += scale[i]*a[i];
    perAtom[atom] += q;
  }
  return true;
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


#ifdef ECCE_HAVE_HDF5

//  The file's atoms must be the ones asked for; centres come from the
//  geometry asked for, as flatBasis() does for the Props.
static bool fromModel(const resultsh5::Model& m, const vector<string>& symbols,
                      const vector<double>& coordsAng,
                      MoAoBasis::OrbitalSet& out)
{
  if (m.natoms() != symbols.size() || coordsAng.size() != 3*symbols.size() ||
      m.label != symbols)
    return false;
  resultsh5::Model c = m;
  for (size_t i = 0; i < coordsAng.size(); i++)
    c.coord[i] = coordsAng[i]*ResultsFromCalc::bohrPerAngstrom();
  string err;
  vector<EspBasisFunction> aos;
  if (!resultsh5::buildAos(c, 0, aos, err) || aos.empty()) return false;
  MoAoBasis::OrbitalSet o;
  o.basis.swap(aos);
  for (size_t k = 0; k < m.mos.size(); k++) {
    const resultsh5::Mos& mo = m.mos[k];
    const int sp = mo.spin & 1;
    if (mo.nao != o.basis.size() || !o.rows[sp].empty()) return false;
    o.rows[sp].assign(mo.nmo, vector<double>(mo.nao));
    for (size_t r = 0; r < mo.nmo; r++)
      std::copy(mo.coef.begin() + r*mo.nao, mo.coef.begin() + (r + 1)*mo.nao,
                o.rows[sp][r].begin());
    o.energy[sp] = mo.energy;
    o.occupation[sp] = mo.occupation;
  }
  if (o.rows[0].empty()) return false;
  o.fromResults = true;
  out = o;
  return true;
}

#endif


bool MoAoBasis::orbitalSet(IPropCalculation *calc,
                           const vector<string>& symbols,
                           const vector<double>& coordsAng, OrbitalSet& out,
                           string *why)
{
  out = OrbitalSet();
#ifdef ECCE_HAVE_HDF5
  {
    std::shared_ptr<const resultsh5::Model> m = ResultsFromCalc::stored(calc);
    if (m && fromModel(*m, symbols, coordsAng, out)) return true;
  }
#endif
  OrbitalSet o;
  if (!coefficients(calc, symbols, false, o.rows[0]) || o.rows[0].empty()) {
    if (why) *why = "The calculation has no MO coefficients.";
    return false;
  }
  if (!coefficients(calc, symbols, true, o.rows[1])) o.rows[1].clear();
  if (!flatBasis(calc, symbols, coordsAng, (int)o.rows[0][0].size(), o.basis)) {
    if (why) *why = "The stored basis set does not match the orbital "
                    "coefficients.";
    return false;
  }
  auto fetch = [&](const char *key, vector<double>& v) {
    PropVector *pv = (PropVector*) calc->getProperty(key);
    v.clear();
    if (pv != 0) for (int i = 0; i < pv->rows(); i++) v.push_back(pv->value(i));
  };
  fetch("ORBENG", o.energy[0]);
  fetch("ORBOCC", o.occupation[0]);
  if (!o.rows[1].empty()) {
    fetch("ORBENGBETA", o.energy[1]);
    fetch("ORBOCCBETA", o.occupation[1]);
  }
  out = o;
  return true;
}


bool MoAoBasis::buildWithOrbitals(IPropCalculation *calc,
                                  const vector<string>& symbols,
                                  const vector<double>& coordsAng,
                                  vector< vector<double> > rows[2],
                                  std::function<void(double)> onRow)
{
#ifdef ECCE_HAVE_HDF5
  std::shared_ptr<const resultsh5::Model> m = ResultsFromCalc::stored(calc);
  OrbitalSet os;
  if (m && fromModel(*m, symbols, coordsAng, os)) {
    elements = symbols;
    perAtom.assign(symbols.size(), 0);
    functions.clear();
    overlap.clear();
    orthonormal = false;
    complaint.clear();
    std::map< std::pair<int,int>, int > seen;
    size_t mu = 0;
    for (size_t sh = 0; sh < m->basis.shells.size(); sh++) {
      const resultsh5::Shell& s = m->basis.shells[sh];
      const int size = s.cartesian ? (s.l + 1)*(s.l + 2)/2 : 2*s.l + 1;
      const int seq = ++seen[std::make_pair(s.atom, s.l)];
      for (int k = 0; k < size; k++, mu++) {
        Function f;
        f.atom = s.atom;
        f.l = s.l;
        f.shellSeq = seq;
        f.component = componentName(os.basis[mu], s.l, k);
        functions.push_back(f);
        perAtom[s.atom]++;
      }
    }
    const size_t n = os.basis.size();
    overlap.assign(n*n, 0.0);
    for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++)
        overlap[i*n + j] = EspField::overlapOf(os.basis[i], os.basis[j]);
      if (onRow) onRow((i + 1.0)/n);
    }
    rows[0] = os.rows[0];
    rows[1] = os.rows[1];
    return true;
  }
#endif
  if (!coefficients(calc, symbols, false, rows[0]) || rows[0].empty())
    return false;
  if (!coefficients(calc, symbols, true, rows[1])) rows[1].clear();
  return build(calc, symbols, coordsAng, (int)rows[0][0].size(), onRow);
}
