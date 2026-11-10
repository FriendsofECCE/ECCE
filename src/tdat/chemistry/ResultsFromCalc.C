// results.h5 for a parsed calculation (#690): see tdat/ResultsFromCalc.H.
#include "tdat/ResultsFromCalc.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "util/Filesystem.H"
#include <map>
#include <memory>

#ifdef ECCE_HAVE_HDF5
#include <unistd.h>

#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropVecTable.H"
#include "tdat/PropValue.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"
#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/TGBSConfig.H"
#endif

namespace ResultsFromCalc
{

bool enabled()
{
  const char *e = getenv("ECCE_RESULTS_H5");
  return !(e != 0 && string(e) == "0");
}

double bohrPerAngstrom() { return 1/0.52917724924; }

#ifdef ECCE_HAVE_HDF5

using resultsh5::Model;
using resultsh5::Shell;

namespace {

int angularMomentum(const EspBasisFunction& f)
{
  if (f.angularCoef.empty()) return -1;
  const int l = f.powerX[0] + f.powerY[0] + f.powerZ[0];
  for (size_t t = 1; t < f.angularCoef.size(); t++)
    if (f.powerX[t] + f.powerY[t] + f.powerZ[t] != l) return -1;
  return l;
}

bool sameRadial(const EspBasisFunction& a, const EspBasisFunction& b)
{
  return a.exponent == b.exponent;
}

//  Overlap with the pairs that cannot reach 1e-16 left out: the checks must
//  not cost more than the Props path's own overlap does.
double screenedOverlap(const EspBasisFunction& a, const EspBasisFunction& b)
{
  double r2 = 0;
  for (int k = 0; k < 3; k++) {
    const double d = a.center[k] - b.center[k];
    r2 += d*d;
  }
  if (r2 > 0 && !a.exponent.empty() && !b.exponent.empty()) {
    const double x = *std::min_element(a.exponent.begin(), a.exponent.end());
    const double y = *std::min_element(b.exponent.begin(), b.exponent.end());
    if (x*y/(x + y)*r2 > 40.0) return 0.0;
  }
  return EspField::overlapOf(a, b);
}

string num(const char *fmt, double v)
{
  char b[64];
  snprintf(b, sizeof b, fmt, v);
  return b;
}


// Factor to hartree / bohr from ECCE's unit names; 0 for one not known, so
// a value in an unexpected unit is left out rather than stored wrong.
double toHartree(const string& u)
{
  if (u == "Hartree" || u == "hartree" || u == "au") return 1.0;
  if (u == "kcal/mol" || u == "kCal/Mole" || u == "kcal/mole") return 1/627.509474;
  if (u == "kJ/mol" || u == "kJoule/Mole" || u == "kJ/mole") return 1/2625.499639;
  if (u == "eV") return 1/27.211386246;
  return 0;
}

double toBohr(const string& u)
{
  if (u == "Angstrom" || u == "angstrom" || u.empty()) return bohrPerAngstrom();
  if (u == "Bohr" || u == "bohr") return 1.0;
  if (u == "nm") return 10*bohrPerAngstrom();
  return 0;
}

PropVector *vectorOf(IPropCalculation *calc, const char *key)
{
  PropVector *v = dynamic_cast<PropVector*>(calc->getProperty(key));
  return v != 0 && v->rows() > 0 ? v : 0;
}

// One trace from a structure property and its per-point companions; a
// companion whose length does not match the structures is left out.
void addTrace(Model& m, const string& name, PropTSVecTable *geom,
              PropVector *energy, PropVector *value, PropVector *time,
              PropTSVecTable *cell)
{
  const size_t N = m.natoms();
  if (geom == 0 || N == 0 || geom->tables() == 0) return;
  const double lf = toBohr(geom->units());
  if (lf == 0) return;
  const size_t np = geom->tables();
  resultsh5::Trace t;
  t.name = name;
  for (size_t i = 0; i < np; i++) {
    //  CREST protomers change the atom count: no fixed-shape trace.
    const vector<double>& x = geom->values((int)i);
    if (x.size() != 3*N) return;
    for (size_t k = 0; k < x.size(); k++) t.coord.push_back(x[k]*lf);
  }
  if (energy != 0 && (size_t)energy->rows() == np) {
    const double ef = toHartree(energy->units());
    if (ef != 0) for (size_t i = 0; i < np; i++) t.energy.push_back(energy->value((int)i)*ef);
  }
  if (value != 0 && (size_t)value->rows() == np) {
    double f = 1.0;
    t.valueUnit = value->units();
    if (t.valueUnit == "Degree") { f = 3.14159265358979323846/180; t.valueUnit = "radian"; }
    else if (t.valueUnit == "Angstrom") { f = bohrPerAngstrom(); t.valueUnit = "bohr"; }
    for (size_t i = 0; i < np; i++) t.value.push_back(value->value((int)i)*f);
  }
  if (time != 0 && (size_t)time->rows() == np)
    for (size_t i = 0; i < np; i++) t.time.push_back(time->value((int)i));
  if (cell != 0 && (size_t)cell->tables() == np && toBohr(cell->units()) != 0) {
    const double cf = toBohr(cell->units());
    for (size_t i = 0; i < np && t.cell.size() == 9*i; i++) {
      const vector<double>& c = cell->values((int)i);
      if (c.size() != 9) { t.cell.clear(); break; }
      for (size_t k = 0; k < 9; k++) t.cell.push_back(c[k]*cf);
    }
  }
  m.trace.push_back(t);
}

// Scans and IRCs: one optimised structure per point.  GEOMTRACE: every
// optimisation step, MD frame (TIMEVEC) or ensemble member (CONFREL,
// POSESCORE); a scan's GEOMTRACE holds all its optimisation steps.
void addTraces(IPropCalculation *calc, Model& m)
{
  auto table = [&](const char *key) {
    return dynamic_cast<PropTSVecTable*>(calc->getProperty(key));
  };
  addTrace(m, "scan", table("SCANGEOM"), vectorOf(calc, "SCANENERGY"),
           vectorOf(calc, "SCANCOORD"), 0, 0);
  addTrace(m, "irc", table("IRCGEOM"), vectorOf(calc, "IRCENERGY"),
           vectorOf(calc, "IRCCOORD"), 0, 0);
  PropTSVecTable *g = table("GEOMTRACE");
  if (g == 0 || g->tables() < 2) return;
  PropVector *time = vectorOf(calc, "TIMEVEC");
  const char *name = time != 0 ? "md"
      : (calc->getProperty("CONFREL") || calc->getProperty("POSESCORE")) ? "ensemble"
      : "optimization";
  addTrace(m, name, g, vectorOf(calc, "TEVEC"), 0, time, table("LATTICEVEC"));
}

}  // namespace


bool convert(const Parsed& in, Model& m, string& why)
{
  m = Model();
  const size_t natoms = in.symbols.size();
  const size_t n = in.basis.size();
  if (natoms == 0 || in.atomicNumber.size() != natoms ||
      in.coordsAng.size() != 3*natoms || n == 0 || in.rows[0].empty()) {
    why = "no atoms, basis or orbitals";
    return false;
  }
  for (int s = 0; s < 2; s++)
    for (size_t r = 0; r < in.rows[s].size(); r++)
      if (in.rows[s][r].size() != n) {
        why = "orbital width does not match the basis";
        return false;
      }
  for (size_t i = 0; i < n; i++)
    if (angularMomentum(in.basis[i]) < 0) {
      why = "an empty or mixed angular function in the basis";
      return false;
    }

  //  Atoms: the flat basis is atom-major, centres exactly those the
  //  functions carry.
  vector<double> centre(3*natoms);
  for (size_t a = 0; a < natoms; a++)
    for (int k = 0; k < 3; k++) centre[3*a + k] = in.coordsAng[3*a + k]*bohrPerAngstrom();
  vector<int> atomOf(n, -1);
  {
    size_t a = 0;
    vector<char> seen(natoms, 0);
    for (size_t i = 0; i < n; i++) {
      size_t b = a;
      for (; b < natoms; b++) {
        double d2 = 0;
        for (int k = 0; k < 3; k++) {
          const double d = in.basis[i].center[k] - centre[3*b + k];
          d2 += d*d;
        }
        if (d2 < 1e-12) break;
      }
      if (b == natoms) { why = "a basis function is not on an atom"; return false; }
      a = b;
      atomOf[i] = (int)a;
      if (!seen[a]) {
        seen[a] = 1;
        for (int k = 0; k < 3; k++) centre[3*a + k] = in.basis[i].center[k];
      }
    }
  }

  //  Shells: consecutive functions of one atom, one l, one set of exponents.
  //  Spherical or Cartesian is read off the first l >= 2 shell: a Cartesian
  //  function is one monomial, a spherical set has z2/x2-y2-like members.
  int cart = -1;
  m.basis.type = "GTO";
  for (size_t i = 0; i < n; ) {
    const int l = angularMomentum(in.basis[i]);
    const size_t ns = 2*l + 1, nc = (l + 1)*(l + 2)/2;
    if (l >= 2 && cart < 0) {
      bool multi = false;
      for (size_t k = i; k < i + ns && k < n; k++)
        if (in.basis[k].angularCoef.size() > 1) multi = true;
      cart = multi ? 0 : 1;
    }
    const size_t size = (l >= 2 && cart == 1) ? nc : ns;
    if (i + size > n) { why = "a shell is cut short"; return false; }
    for (size_t k = i + 1; k < i + size; k++)
      if (atomOf[k] != atomOf[i] || angularMomentum(in.basis[k]) != l ||
          !sameRadial(in.basis[k], in.basis[i])) {
        why = "functions of one shell do not form a shell";
        return false;
      }
    Shell s;
    s.atom = atomOf[i];
    s.l = l;
    s.cartesian = (l >= 2 && cart == 1);
    s.exponent = in.basis[i].exponent;
    s.coef = in.basis[i].contraction;
    m.basis.shells.push_back(s);
    i += size;
  }

  m.atomicNumber = in.atomicNumber;
  m.coord = centre;
  m.geomEnergy.clear();
  for (size_t a = 0; a < natoms; a++) m.label.push_back(in.symbols[a]);

  //  Canonical AOs against ECCE's: each flat function is a multiple of one
  //  canonical function of its shell (a permutation, a sign, a scale).
  vector<EspBasisFunction> can;
  string err;
  if (!resultsh5::buildAos(m, 0, can, err) || can.size() != n) {
    why = "canonical basis: " + err;
    return false;
  }
  vector<size_t> to(n);
  vector<double> scale(n);
  {
    size_t i = 0;
    for (size_t sh = 0; sh < m.basis.shells.size(); sh++) {
      const Shell& s = m.basis.shells[sh];
      const size_t size = s.cartesian ? (s.l + 1)*(s.l + 2)/2 : 2*s.l + 1;
      vector<char> used(size, 0);
      for (size_t t = 0; t < size; t++) {
        const double self = EspField::overlapOf(in.basis[i + t], in.basis[i + t]);
        size_t best = size;
        double bov = 0;
        for (size_t j = 0; j < size; j++) {
          const double ov = EspField::overlapOf(in.basis[i + t], can[i + j]);
          if (std::fabs(ov) > std::fabs(bov)) { bov = ov; best = j; }
        }
        if (best == size || !(self > 0) ||
            1 - bov*bov/self > 1e-8 || used[best]) {
          why = "a basis function is not a canonical function of its shell";
          return false;
        }
        used[best] = 1;
        to[i + t] = i + best;
        scale[i + t] = bov;
      }
      i += size;
    }
  }

  for (int sp = 0; sp < 2; sp++) {
    const vector<vector<double> >& rows = in.rows[sp];
    if (rows.empty()) continue;
    resultsh5::Mos mo;
    mo.spin = sp;
    mo.geometry = 0;
    mo.nmo = rows.size();
    mo.nao = n;
    mo.coef.assign(mo.nmo*n, 0.0);
    for (size_t r = 0; r < mo.nmo; r++)
      for (size_t i = 0; i < n; i++) mo.coef[r*n + to[i]] = rows[r][i]*scale[i];
    if (in.energy[sp].size() == mo.nmo) mo.energy = in.energy[sp];
    mo.occupation = in.occupation[sp];
    mo.occupation.resize(mo.nmo, 0.0);
    m.mos.push_back(mo);
  }

  if (in.rows[1].empty()) {
    double a = 0, b = 0;
    for (size_t i = 0; i < in.occupation[0].size(); i++) {
      a += std::min(in.occupation[0][i], 1.0);
      b += std::max(in.occupation[0][i] - 1.0, 0.0);
    }
    m.nelectron[0] = (int)(a + 0.5);
    m.nelectron[1] = (int)(b + 0.5);
  } else {
    for (int sp = 0; sp < 2; sp++) {
      double a = 0;
      for (size_t i = 0; i < in.occupation[sp].size(); i++) a += in.occupation[sp][i];
      m.nelectron[sp] = (int)(a + 0.5);
    }
  }

  m.provenance["code"] = in.code;
  if (!in.codeVersion.empty()) m.provenance["code_version"] = in.codeVersion;
  if (!in.method.empty()) m.provenance["method"] = in.method;
  if (!in.basisName.empty()) m.provenance["basis_name"] = in.basisName;
  //  Every provenance attribute costs about 0.2 kB, which a 3-atom file feels.
  m.provenance["converter_version"] = resultsh5::CONVERTER_VERSION;
  return true;
}


bool gather(IPropCalculation *calc, Parsed& out, string& why)
{
  out = Parsed();
  if (calc == 0) { why = "no calculation"; return false; }
  if (calc->getProperty("MO") == 0) { why = "no MO coefficients"; return false; }
  std::unique_ptr<Fragment> own(calc->getFragmentStep(-1));
  Fragment *frag = own.get();
  if (frag == 0 || frag->numAtoms() == 0) {
    why = "no chemical system";
    return false;
  }
  const bool geometry = MoAoBasis::fragmentGeometry(frag, out.symbols, out.coordsAng);
  if (geometry) {
    vector<TAtm*> *atoms = frag->atoms();
    if (atoms != 0) {
      for (size_t a = 0; a < atoms->size(); a++)
        out.atomicNumber.push_back((*atoms)[a]->atomicNumber());
      delete atoms;
    }
  }
  if (!geometry || out.atomicNumber.size() != out.symbols.size()) {
    why = "the chemical system has no usable atoms";
    return false;
  }

  if (!MoAoBasis::coefficients(calc, out.symbols, false, out.rows[0]) ||
      out.rows[0].empty()) {
    why = "no MO coefficients";
    return false;
  }
  if (calc->getProperty("MOBETA") != 0)
    MoAoBasis::coefficients(calc, out.symbols, true, out.rows[1]);
  if (!MoAoBasis::flatBasis(calc, out.symbols, out.coordsAng,
                            (int)out.rows[0][0].size(), out.basis)) {
    why = "the stored basis set does not match the orbital coefficients "
          "(or is not Gaussian)";
    return false;
  }
  bool semi = false;
  vector<int> counts, shellOf;
  if (MoAoBasis::functionCounts(calc, out.symbols, (int)out.rows[0][0].size(),
                                counts, shellOf, &semi) && semi) {
    why = "an orthonormal semiempirical basis has no Gaussian shells";
    return false;
  }

  auto fetch = [&](const char *key, vector<double>& v) {
    PropVector *pv = dynamic_cast<PropVector*>(calc->getProperty(key));
    v.clear();
    if (pv != 0) for (int i = 0; i < pv->rows(); i++) v.push_back(pv->value(i));
  };
  fetch("ORBENG", out.energy[0]);
  fetch("ORBOCC", out.occupation[0]);
  if (!out.rows[1].empty()) {
    fetch("ORBENGBETA", out.energy[1]);
    fetch("ORBOCCBETA", out.occupation[1]);
  }
  if (out.occupation[0].empty()) {
    why = "no orbital occupations";
    return false;
  }

  ICalculation *ic = dynamic_cast<ICalculation*>(calc);
  if (ic != 0) {
    const JCode *code = ic->application();
    if (code != 0) out.code = code->name();
    TTheory *th = ic->theory();
    if (th != 0) { out.method = th->toConciseString(); delete th; }
    TGBSConfig *gbs = ic->gbsConfig();
    if (gbs != 0) { out.basisName = gbs->name(); delete gbs; }
  }
  return true;
}


void addProperties(IPropCalculation *calc, Model& m)
{
  const size_t N = m.natoms();
  PropValue *te = dynamic_cast<PropValue*>(calc->getProperty("TE"));
  if (te != 0 && m.nframes() == 1) m.geomEnergy.push_back(te->value());

  static const char *const schemes[][2] = {
    {"MULLIKEN", "mulliken"}, {"LOEWDIN", "loewdin"}, {"HIRSHFELD", "hirshfeld"},
    {"CHELPG", "chelpg"}, {"MK", "mk"}, {"ESPCHARGE", "esp"}
  };
  for (size_t k = 0; k < sizeof schemes/sizeof schemes[0]; k++) {
    PropVector *pv = dynamic_cast<PropVector*>(calc->getProperty(schemes[k][0]));
    if (pv == 0 || pv->rows() != (int)N) continue;
    resultsh5::Charges c;
    c.scheme = schemes[k][1];
    for (int i = 0; i < pv->rows(); i++) c.q.push_back(pv->value(i));
    m.charges.push_back(c);
  }

  //  ECCE keeps the dipole in Debye.
  PropVector *dip = dynamic_cast<PropVector*>(calc->getProperty("DIPOLE"));
  if (dip != 0 && dip->rows() == 3) {
    resultsh5::Multipoles p;
    for (int i = 0; i < 3; i++) p.dipole.push_back(dip->value(i)/2.541746473);
    m.multipoles.push_back(p);
  }

  PropTable *grad = dynamic_cast<PropTable*>(calc->getProperty("EGRADVEC"));
  if (grad != 0 && grad->rows() == (int)N && grad->columns() == 3) {
    resultsh5::Gradient g;
    for (int i = 0; i < grad->rows(); i++)
      for (int c = 0; c < 3; c++) g.g.push_back(grad->value(i, c));
    m.gradient.push_back(g);
  }

  PropVector *fr = dynamic_cast<PropVector*>(calc->getProperty("VIBFREQ"));
  if (fr != 0 && fr->rows() > 0) {
    resultsh5::Vib v;
    for (int i = 0; i < fr->rows(); i++) v.frequency.push_back(fr->value(i));
    PropVector *ir = dynamic_cast<PropVector*>(calc->getProperty("VIBIR"));
    if (ir != 0 && ir->rows() == fr->rows())
      for (int i = 0; i < ir->rows(); i++) v.irIntensity.push_back(ir->value(i));
    PropVector *ram = dynamic_cast<PropVector*>(calc->getProperty("VIBRAM"));
    if (ram != 0 && ram->rows() == fr->rows())
      for (int i = 0; i < ram->rows(); i++) v.ramanActivity.push_back(ram->value(i));
    //  One displacement table per frequency, one row per atom, or none.
    PropVecTable *modes = dynamic_cast<PropVecTable*>(calc->getProperty("VIB"));
    if (modes != 0 && modes->tables() == fr->rows() && modes->rows() == (int)N &&
        modes->columns() == 3)
      for (int k = 0; k < modes->tables(); k++) {
        const vector<double>& t = modes->values(k);
        v.mode.insert(v.mode.end(), t.begin(), t.end());
      }
    m.vib.push_back(v);
  }
  addTraces(calc, m);
}


string check(Model& m)
{
  vector<EspBasisFunction> aos;
  string err;
  if (m.mos.empty() || !resultsh5::buildAos(m, 0, aos, err)) {
    m.checks = "failed: " + (err.empty() ? string("no orbitals") : err);
    return "checks failed (" + (err.empty() ? string("no orbitals") : err) + ")";
  }
  const size_t n = aos.size();
  vector<double> S(n*n, 0.0);
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j <= i; j++) S[i*n + j] = S[j*n + i] = screenedOverlap(aos[i], aos[j]);

  double traceErr = 0, orthoErr = 0;
  string perSpin;
  for (size_t k = 0; k < m.mos.size(); k++) {
    const resultsh5::Mos& mo = m.mos[k];
    vector<size_t> occ;
    double N = 0, tr = 0;
    for (size_t i = 0; i < mo.nmo; i++)
      if (mo.occupation[i] > 1e-8) { occ.push_back(i); N += mo.occupation[i]; }
    vector<vector<double> > sc(occ.size(), vector<double>(n, 0.0));
    for (size_t a = 0; a < occ.size(); a++) {
      const double *c = &mo.coef[occ[a]*n];
      for (size_t i = 0; i < n; i++) {
        double x = 0;
        for (size_t j = 0; j < n; j++) x += S[i*n + j]*c[j];
        sc[a][i] = x;
      }
    }
    for (size_t a = 0; a < occ.size(); a++) {
      for (size_t b = 0; b <= a; b++) {
        const double *c = &mo.coef[occ[b]*n];
        double x = 0;
        for (size_t i = 0; i < n; i++) x += c[i]*sc[a][i];
        if (a == b) tr += mo.occupation[occ[a]]*x;
        orthoErr = std::max(orthoErr, std::fabs(x - (a == b ? 1.0 : 0.0)));
      }
    }
    const double rel = N > 0 ? std::fabs(tr - N)/N : 1.0;
    traceErr = std::max(traceErr, rel);
    perSpin += (perSpin.empty() ? "" : "; ") + string(mo.spin ? "beta" : "alpha") +
               " tr(PS) " + num("%.6f", tr) + " of " + num("%.4f", N);
  }
  const bool ok = traceErr <= 1e-4 && orthoErr <= 1e-5;
  const string numbers = perSpin + ", rel err " + num("%.1e", traceErr) +
                         ", max |C'SC-1| " + num("%.1e", orthoErr) + ", " +
                         std::to_string(n) + " AOs";
  m.checks = (ok ? "ok: " : "failed: ") + numbers;
  return string("checks ") + (ok ? "passed" : "FAILED (file marked, views ignore it)") +
         ": " + numbers;
}


namespace {

// Atoms and final geometry only: for a calculation without Gaussian
// orbitals (MD, MOPAC, force fields) whose vibrations or traces are kept.
bool geometryOnly(IPropCalculation *calc, Model& m)
{
  std::unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  vector<string> symbols;
  vector<double> xyz;
  if (!frag || frag->numAtoms() == 0 ||
      !MoAoBasis::fragmentGeometry(frag.get(), symbols, xyz))
    return false;
  m = Model();
  vector<TAtm*> *atoms = frag->atoms();
  if (atoms == 0) return false;
  for (size_t a = 0; a < atoms->size(); a++)
    m.atomicNumber.push_back((*atoms)[a]->atomicNumber());
  delete atoms;
  if (m.atomicNumber.size() != symbols.size()) return false;
  m.label = symbols;
  for (size_t i = 0; i < xyz.size(); i++) m.coord.push_back(xyz[i]*bohrPerAngstrom());
  m.basis.type = "none";
  ICalculation *ic = dynamic_cast<ICalculation*>(calc);
  if (ic != 0 && ic->application() != 0) m.provenance["code"] = ic->application()->name();
  m.provenance["converter_version"] = resultsh5::CONVERTER_VERSION;
  return true;
}

}  // namespace


bool build(IPropCalculation *calc, Model& m, string& line, string& why)
{
  Parsed in;
  if (!gather(calc, in, why) || !convert(in, m, why)) {
    Model g;
    if (!geometryOnly(calc, g)) return false;
    addProperties(calc, g);
    if (g.vib.empty() && g.trace.empty()) return false;
    g.checks = "ok: no orbitals (" + why + ")";
    m = g;
    line = "no orbitals (" + why + "): atoms, vibrations and traces only";
    return true;
  }
  addProperties(calc, m);
  line = check(m);
  return true;
}


namespace {

string scratchFile()
{
  static int counter = 0;
  std::error_code ec;
  ecce::fs::path dir = ecce::fs::temp_directory_path(ec);
  if (ec) dir = ".";
  return (dir / ("ecce-results-" + std::to_string((long)getpid()) + "-" +
                 std::to_string(++counter) + ".h5")).string();
}

}  // namespace


bool store(IPropCalculation *calc, string& line)
{
  if (!enabled()) { line = "results.h5 is switched off (ECCE_RESULTS_H5=0)"; return false; }
  //  ECCE-QM writes the file from its own full-precision data, with the
  //  Hessian the parsed properties lack; one that passes the same checks is
  //  kept, as the properties cannot reproduce it.
  {
    const string own = scratchFile();
    Model q;
    string err;
    bool keep = false;
    if (calc->fetchStoredFile(FILE_NAME, own) && resultsh5::readFile(own, q, err) &&
        q.provenance.count("writer") && q.provenance["writer"] == "ecce-qm") {
      const string l = check(q);
      if (q.checks.compare(0, 3, "ok:") == 0) {
        keep = true;
        line = "results.h5 written by ECCE-QM kept; " + l;
      }
    }
    std::remove(own.c_str());
    if (keep) { forget(calc); return true; }
  }
  Model m;
  string why;
  if (!build(calc, m, line, why)) {
    calc->removeStoredFile(FILE_NAME);
    line = "results.h5 not written: " + why;
    return false;
  }
  const string path = scratchFile();
  string err;
  bool ok = resultsh5::writeFile(path, m, err);
  if (ok) {
    ok = calc->storeFile(FILE_NAME, path);
    if (!ok) err = "the data store did not take the file";
  }
  std::remove(path.c_str());
  forget(calc);
  if (!ok) { line = "results.h5 not stored: " + err; return false; }
  line = "results.h5 stored; " + line;
  return true;
}


namespace {

struct Cache
{
  IPropCalculation *calc;
  string url;
  time_t when;
  std::shared_ptr<const Model> model;
  Cache() : calc(0), when(0) {}
};
Cache g_cache;

}  // namespace


void forget(IPropCalculation *calc)
{
  if (g_cache.calc == calc) g_cache = Cache();
}


std::shared_ptr<const Model> stored(IPropCalculation *calc)
{
  if (calc == 0 || !enabled() || calc->orbitalPoint() != -1)
    return std::shared_ptr<const Model>();
  const string url = calc->getURL().toString();
  const time_t now = time(0);
  //  The age limit covers a file written by another process (a re-parse);
  //  the calculation's own reloads call forget().
  if (g_cache.calc == calc && g_cache.url == url && now - g_cache.when < 60)
    return g_cache.model;

  g_cache = Cache();
  g_cache.calc = calc;
  g_cache.url = url;
  g_cache.when = now;
  const string path = scratchFile();
  if (calc->fetchStoredFile(FILE_NAME, path)) {
    Model *m = new Model();
    string err;
    if (resultsh5::readFile(path, *m, err) && !m->mos.empty() &&
        m->checks.compare(0, 6, "failed") != 0)
      g_cache.model.reset(m);
    else
      delete m;
  }
  std::remove(path.c_str());
  return g_cache.model;
}

#else   // no HDF5: no results.h5

bool convert(const Parsed&, resultsh5::Model&, string& why) { why = "built without HDF5"; return false; }
bool gather(IPropCalculation*, Parsed&, string& why) { why = "built without HDF5"; return false; }
void addProperties(IPropCalculation*, resultsh5::Model&) {}
string check(resultsh5::Model&) { return string(); }
bool build(IPropCalculation*, resultsh5::Model&, string&, string& why) { why = "built without HDF5"; return false; }
bool store(IPropCalculation*, string& line) { line = "built without HDF5"; return false; }
std::shared_ptr<const resultsh5::Model> stored(IPropCalculation*) { return std::shared_ptr<const resultsh5::Model>(); }
void forget(IPropCalculation*) {}

#endif

}
