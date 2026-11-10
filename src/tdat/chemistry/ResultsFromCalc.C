// results.h5 for a parsed calculation (#690): see tdat/ResultsFromCalc.H.
#include "tdat/ResultsFromCalc.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ctime>
#include "util/Filesystem.H"
#include <map>
#include <memory>
#include <numeric>

#include "tdat/PropTSVecTable.H"
#include "tdat/PropVector.H"
#include "dsm/IPropCalculation.H"

#ifdef ECCE_HAVE_HDF5
#include <strings.h>
#include <unistd.h>

#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropVecTable.H"
#include "tdat/PropValue.H"
#include "tdat/PropVector.H"
#include "tdat/PropTSVector.H"
#include "tdat/TPerTab.H"
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

namespace {

//  An IRC's points come in run order (Gaussian: TS, forward, reverse) and
//  grow while the job runs (#948); views and results.h5 want the path from
//  one end to the other, by reaction coordinate.  A frame's index is its
//  energy's, so all three keys are reordered together.
struct IrcPath
{
  PropVector *coord = 0, *energy = 0;
  PropTSVecTable *geom = 0;
  vector<size_t> order;
  vector<vector<double> > frames;   // in path order, TS fitted
};

//  Rotates the TS (printed in the orientation it was read in) rigidly onto
//  the mean of its neighbours (Horn's quaternion fit), when that fits clearly
//  better than as is; distances are unchanged.
void fitTs(vector<double>& ts, const vector<const vector<double>*>& near)
{
  const size_t n = ts.size()/3;
  if (n < 2 || near.empty()) return;
  vector<double> b(ts.size(), 0.0);
  for (const vector<double> *f : near) {
    if (f->size() != ts.size()) return;
    for (size_t k = 0; k < ts.size(); k++) b[k] += (*f)[k]/near.size();
  }
  double ca[3] = {0, 0, 0}, cb[3] = {0, 0, 0};
  for (size_t i = 0; i < n; i++)
    for (int k = 0; k < 3; k++) { ca[k] += ts[3*i+k]/n; cb[k] += b[3*i+k]/n; }
  double S[3][3] = {{0}};
  for (size_t i = 0; i < n; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 3; k++) S[j][k] += (ts[3*i+j] - ca[j])*(b[3*i+k] - cb[k]);
  const double xx = S[0][0], xy = S[0][1], xz = S[0][2], yx = S[1][0], yy = S[1][1],
               yz = S[1][2], zx = S[2][0], zy = S[2][1], zz = S[2][2];
  const double N[4][4] = {{xx + yy + zz, yz - zy, zx - xz, xy - yx},
                          {yz - zy, xx - yy - zz, xy + yx, zx + xz},
                          {zx - xz, xy + yx, -xx + yy - zz, yz + zy},
                          {xy - yx, zx + xz, yz + zy, -xx - yy + zz}};
  //  Power iteration on N + shift*I gives the top eigenvector; a generic
  //  start, as a planar molecule decouples N into 2x2 blocks.
  double shift = 0;
  for (auto& r : N) for (double v : r) shift += v*v;
  shift = sqrt(shift) + 1e-12;
  double q[4] = {0.8, 0.37, 0.33, 0.29};
  for (int it = 0; it < 2000; it++) {
    double v[4], len = 0;
    for (int r = 0; r < 4; r++) {
      v[r] = shift*q[r];
      for (int c = 0; c < 4; c++) v[r] += N[r][c]*q[c];
      len += v[r]*v[r];
    }
    len = sqrt(len);
    for (int r = 0; r < 4; r++) q[r] = v[r]/len;
  }
  const double q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
  const double R[3][3] = {
    {q0*q0 + q1*q1 - q2*q2 - q3*q3, 2*(q1*q2 - q0*q3), 2*(q1*q3 + q0*q2)},
    {2*(q1*q2 + q0*q3), q0*q0 - q1*q1 + q2*q2 - q3*q3, 2*(q2*q3 - q0*q1)},
    {2*(q1*q3 - q0*q2), 2*(q2*q3 + q0*q1), q0*q0 - q1*q1 - q2*q2 + q3*q3}};
  vector<double> out(ts.size());
  double dFit = 0, dAsIs = 0;
  for (size_t i = 0; i < n; i++) {
    const double d[3] = {ts[3*i] - ca[0], ts[3*i+1] - ca[1], ts[3*i+2] - ca[2]};
    for (int k = 0; k < 3; k++) {
      out[3*i+k] = cb[k] + R[k][0]*d[0] + R[k][1]*d[1] + R[k][2]*d[2];
      dFit += (out[3*i+k] - b[3*i+k])*(out[3*i+k] - b[3*i+k]);
      dAsIs += (ts[3*i+k] - b[3*i+k])*(ts[3*i+k] - b[3*i+k]);
    }
  }
  if (dFit*4 < dAsIs) ts.swap(out);
}

//  False unless there is one structure and one energy per coordinate.
bool ircPath(PropVector *c, PropVector *e, PropTSVecTable *g, IrcPath& path)
{
  if (c == 0 || e == 0 || g == 0) return false;
  const size_t n = c->values().size();
  if (n == 0 || e->values().size() != n || (size_t)g->tables() != n) return false;
  const vector<double>& x = c->values();
  path.coord = c; path.energy = e; path.geom = g;
  path.order.resize(n);
  std::iota(path.order.begin(), path.order.end(), 0);
  std::stable_sort(path.order.begin(), path.order.end(),
                   [&](size_t a, size_t b) { return x[a] < x[b]; });
  path.frames.clear();
  for (size_t i : path.order) path.frames.push_back(g->values((int)i));
  //  The TS is the point at coordinate 0; its neighbours the nearest points
  //  on either side.
  size_t ts = 0;
  for (size_t i = 1; i < n; i++) if (fabs(x[path.order[i]]) < fabs(x[path.order[ts]])) ts = i;
  if (x[path.order[ts]] != 0) return true;
  vector<const vector<double>*> near;
  if (ts > 0) near.push_back(&path.frames[ts - 1]);
  if (ts + 1 < n) near.push_back(&path.frames[ts + 1]);
  fitTs(path.frames[ts], near);
  return true;
}

vector<double> inOrder(const vector<double>& v, const vector<size_t>& order)
{
  vector<double> out;
  for (size_t i : order) out.push_back(v[i]);
  return out;
}

//  A copy of one of the path's three properties in path order.
TProperty *ircCopy(const IrcPath& path, const string& key)
{
  if (key == "IRCGEOM") {
    PropTSVecTable *t = new PropTSVecTable(*path.geom);
    t->setValues(new vector<vector<double> >(path.frames), path.geom->rows(), 3);
    return t;
  }
  PropVector *v = new PropVector(key == "IRCCOORD" ? *path.coord : *path.energy);
  v->values(inOrder(v->values(), path.order));
  return v;
}

}  // namespace

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
  //  "NA": a structure table whose parser declared no unit (ORCA's
  //  IRCGEOM); ECCE's coordinates are Angstrom.
  if (u == "Angstrom" || u == "angstrom" || u.empty() || u == "NA") return bohrPerAngstrom();
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
  IrcPath path;
  if (ircPath(vectorOf(calc, "IRCCOORD"), vectorOf(calc, "IRCENERGY"), table("IRCGEOM"), path)) {
    std::unique_ptr<TProperty> g(ircCopy(path, "IRCGEOM")), e(ircCopy(path, "IRCENERGY")),
        c(ircCopy(path, "IRCCOORD"));
    addTrace(m, "irc", dynamic_cast<PropTSVecTable*>(g.get()), dynamic_cast<PropVector*>(e.get()),
             dynamic_cast<PropVector*>(c.get()), 0, 0);
  } else {
    addTrace(m, "irc", table("IRCGEOM"), vectorOf(calc, "IRCENERGY"),
             vectorOf(calc, "IRCCOORD"), 0, 0);
  }
  PropTSVecTable *g = table("GEOMTRACE");
  if (g == 0 || g->tables() < 2) return;
  PropVector *time = vectorOf(calc, "TIMEVEC");
  const char *name = time != 0 ? "md"
      : (calc->getProperty("CONFREL") || calc->getProperty("POSESCORE")) ? "ensemble"
      : "optimization";
  addTrace(m, name, g, vectorOf(calc, "TEVEC"), 0, time, table("LATTICEVEC"));
}


string scratchFile()
{
  static int counter = 0;
  std::error_code ec;
  ecce::fs::path dir = ecce::fs::temp_directory_path(ec);
  if (ec) dir = ".";
  return (dir / ("ecce-results-" + std::to_string((long)getpid()) + "-" +
                 std::to_string(++counter) + ".h5")).string();
}

// NWChem's movecs file (Fortran unformatted, 4-byte record markers): header
// records, then nsets, nbf, nmo per set, and per set occupations, energies
// and one record per MO.  Integers are 4 or 8 bytes depending on the build.
bool readMovecs(const string& path, vector<vector<vector<double> > >& sets)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  vector<string> recs;
  int32_t n = 0;
  while (in.read(reinterpret_cast<char*>(&n), 4)) {
    if (n < 0 || n > (1 << 28)) return false;
    string r(n, '\0');
    int32_t tail = 0;
    if (!in.read(&r[0], n) || !in.read(reinterpret_cast<char*>(&tail), 4) || tail != n)
      return false;
    recs.push_back(r);
  }
  auto integer = [&](size_t k, size_t i, long& v) {
    const string& r = recs[k];
    if (r.size() % 8 == 0 && r.size() >= 8*(i + 1)) { int64_t x; memcpy(&x, &r[8*i], 8); v = (long)x; }
    else if (r.size() % 4 == 0 && r.size() >= 4*(i + 1)) { int32_t x; memcpy(&x, &r[4*i], 4); v = x; }
    else return false;
    return true;
  };
  // records 0-5: checksum, scftype, title length, title, basis length, basis name
  long nsets = 0, nbf = 0;
  if (recs.size() < 9 || !integer(6, 0, nsets) || !integer(7, 0, nbf) ||
      nsets < 1 || nsets > 2 || nbf < 1)
    return false;
  // with 8-byte integers nmo has one entry per set: tell 4 from 8 by size
  const bool i8 = recs[8].size() == 8*(size_t)nsets;
  size_t k = 9;
  sets.assign(nsets, vector<vector<double> >());
  for (long s = 0; s < nsets; s++) {
    long nmo = 0;
    if (i8) { int64_t x; memcpy(&x, &recs[8][8*s], 8); nmo = (long)x; }
    else if (recs[8].size() >= 4*(size_t)(s + 1)) { int32_t x; memcpy(&x, &recs[8][4*s], 4); nmo = x; }
    if (nmo < 1 || nmo > nbf || k + 2 + nmo > recs.size()) return false;
    k += 2;                                   // occupations, energies
    for (long j = 0; j < nmo; j++, k++) {
      if (recs[k].size() != 8*(size_t)nbf) return false;
      vector<double> v(nbf);
      memcpy(v.data(), recs[k].data(), 8*nbf);
      sets[s].push_back(v);
    }
  }
  return true;
}

// NWChem's eprint holds the MO coefficients to 3 decimals, which fails
// tr(PS) = N by about 1e-3 (#944).  The job keeps nwchem.movecs at full
// precision; use it when it is the same set of orbitals as the parsed one.
bool fullPrecisionNwchem(IPropCalculation *calc, Parsed& out)
{
  const string tmp = scratchFile();
  vector<vector<vector<double> > > sets;
  const bool read = calc->fetchStoredFile("nwchem.movecs", tmp) && readMovecs(tmp, sets);
  unlink(tmp.c_str());
  const size_t nspin = out.rows[1].empty() ? 1 : 2;
  if (!read || sets.size() != nspin) return false;
  for (size_t sp = 0; sp < nspin; sp++) {
    const vector<vector<double> >& rows = out.rows[sp];
    if (sets[sp].size() != rows.size()) return false;
    for (size_t r = 0; r < rows.size(); r++) {
      if (sets[sp][r].size() != rows[r].size()) return false;
      for (size_t i = 0; i < rows[r].size(); i++)
        if (std::fabs(sets[sp][r][i] - rows[r][i]) > 6e-4) return false;
    }
  }
  for (size_t sp = 0; sp < nspin; sp++) out.rows[sp] = sets[sp];
  return true;
}

//  Each flat function is a multiple of one canonical function of its shell
//  (a permutation, a sign and a scale): flat[i] = canonical[to[i]]*scale[i]
//  in coefficient terms.  The shells of m must follow the flat order.
bool matchShells(const vector<EspBasisFunction>& flat, Model& m,
                 vector<size_t>& to, vector<double>& scale, string& why)
{
  const size_t n = flat.size();
  //  Canonical functions sit on the flat ones' centres, so a geometry
  //  stored to fewer digits cannot spoil the match.
  size_t at = 0;
  for (const Shell& s : m.basis.shells) {
    if (at >= n) { why = "the basis has more shells than functions"; return false; }
    if (s.atom >= 0 && 3*(size_t)s.atom + 2 < m.coord.size())
      for (int k = 0; k < 3; k++) m.coord[3*s.atom + k] = flat[at].center[k];
    at += s.cartesian ? (s.l + 1)*(s.l + 2)/2 : 2*s.l + 1;
  }
  vector<EspBasisFunction> can;
  string err;
  if (!resultsh5::buildAos(m, 0, can, err) || can.size() != n) {
    why = "canonical basis: " + (err.empty() ? string("size differs") : err);
    return false;
  }
  to.assign(n, 0);
  scale.assign(n, 0.0);
  size_t i = 0;
  for (size_t sh = 0; sh < m.basis.shells.size(); sh++) {
    const Shell& s = m.basis.shells[sh];
    const size_t size = s.cartesian ? (s.l + 1)*(s.l + 2)/2 : 2*s.l + 1;
    vector<char> used(size, 0);
    for (size_t t = 0; t < size; t++) {
      const double self = EspField::overlapOf(flat[i + t], flat[i + t]);
      size_t best = size;
      double bov = 0;
      for (size_t j = 0; j < size; j++) {
        const double ov = EspField::overlapOf(flat[i + t], can[i + j]);
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
  return true;
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

  vector<size_t> to;
  vector<double> scale;
  if (!matchShells(in.basis, m, to, scale, why)) return false;

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
  if (out.code == "NWChem") fullPrecisionNwchem(calc, out);
  return true;
}


bool codeColumns(IPropCalculation *calc, const Model& file,
                 vector<vector<double> > rows[2], string& why)
{
  rows[0].clear();
  rows[1].clear();
  if (calc == 0 || file.mos.empty()) { why = "no orbitals in results.h5"; return false; }
  std::unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  vector<string> symbols;
  vector<double> coords;
  if (frag == 0 || !MoAoBasis::fragmentGeometry(frag.get(), symbols, coords)) {
    why = "no chemical system";
    return false;
  }
  const size_t n = file.mos[0].nao;
  vector<EspBasisFunction> flat;
  if (!MoAoBasis::flatBasis(calc, symbols, coords, (int)n, flat) ||
      flat.size() != n) {
    why = "the calculation's basis set does not match results.h5";
    return false;
  }
  Model m = file;
  vector<size_t> to;
  vector<double> scale;
  if (!matchShells(flat, m, to, scale, why)) return false;
  for (const resultsh5::Mos& mo : file.mos) {
    if (mo.geometry != 0 || mo.spin < 0 || mo.spin > 1 || mo.nao != n ||
        mo.coef.size() != mo.nmo*n || !rows[mo.spin].empty())
      continue;
    vector<vector<double> >& out = rows[mo.spin];
    out.assign(mo.nmo, vector<double>(n));
    for (size_t r = 0; r < mo.nmo; r++)
      for (size_t i = 0; i < n; i++) out[r][i] = mo.coef[r*n + to[i]]/scale[i];
  }
  if (rows[0].empty()) { why = "no alpha orbitals in results.h5"; return false; }
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
    // An empty channel (the beta of H2+) has tr(PS) = 0 exactly: compare it
    // absolutely, since there is no N to divide by.
    const double rel = N > 0 ? std::fabs(tr - N)/N : std::fabs(tr);
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
//  Calculations without a file: views and Organizer tables ask for missing
//  keys of many calculations in turn, and the one-entry cache would list
//  each calculation's outputs again.
std::map<string, time_t> g_noFile;

}  // namespace


void forget(IPropCalculation *calc)
{
  if (calc) g_noFile.erase(calc->getURL().toString());
  if (g_cache.calc == calc) g_cache = Cache();
}


std::shared_ptr<const Model> storedAny(IPropCalculation *calc)
{
  if (calc == 0 || !enabled()) return std::shared_ptr<const Model>();
  const string url = calc->getURL().toString();
  const time_t now = time(0);
  //  The age limit covers a file written by another process (a re-parse);
  //  the calculation's own reloads call forget().
  if (g_cache.calc == calc && g_cache.url == url && now - g_cache.when < 60)
    return g_cache.model;

  auto none = g_noFile.find(url);
  if (none != g_noFile.end() && now - none->second < 60) return std::shared_ptr<const Model>();

  g_cache = Cache();
  g_cache.calc = calc;
  g_cache.url = url;
  g_cache.when = now;
  const string path = scratchFile();
  if (!calc->fetchStoredFile(FILE_NAME, path)) g_noFile[url] = now;
  else {
    Model *m = new Model();
    string err;
    if (resultsh5::readFile(path, *m, err) &&
        m->checks.compare(0, 6, "failed") != 0)
      g_cache.model.reset(m);
    else
      delete m;
  }
  std::remove(path.c_str());
  return g_cache.model;
}


std::shared_ptr<const Model> stored(IPropCalculation *calc)
{
  if (calc == 0 || calc->orbitalPoint() != -1) return std::shared_ptr<const Model>();
  std::shared_ptr<const Model> m = storedAny(calc);
  return m && !m->mos.empty() ? m : std::shared_ptr<const Model>();
}


namespace {

const resultsh5::Trace *traceNamed(const Model& m, const string& key)
{
  for (const resultsh5::Trace& t : m.trace) {
    const bool step = t.name == "optimization" || t.name == "md" || t.name == "ensemble";
    if ((key.compare(0, 4, "SCAN") == 0 && t.name == "scan") ||
        (key.compare(0, 3, "IRC") == 0 && t.name == "irc") ||
        ((key == "GEOMTRACE" || key == "TEVEC") && step))
      return &t;
  }
  return 0;
}

// Factor from the file's coordinate unit to the property's; 0 when the two
// are not known to match.
double valueFactor(const string& file, const string& prop)
{
  if (file == "radian" && prop == "Degree") return 180/3.14159265358979323846;
  if (file == "bohr" && prop == "Angstrom") return 1/bohrPerAngstrom();
  if (strcasecmp(file.c_str(), prop.c_str()) == 0) return 1.0;
  return 0;
}

// The values of `key` from the file in the shape and units of `p`, or false.
bool valuesFromFile(const Model& m, const string& key, TProperty *p,
                    vector<double>& out)
{
  out.clear();
  const size_t N = m.natoms();
  if (PropVector *v = dynamic_cast<PropVector*>(p)) {
    const size_t n = v->rows();
    const vector<double> *src = 0;
    double f = 1.0;
    if (!m.vib.empty() && key == "VIBFREQ") src = &m.vib[0].frequency;
    else if (!m.vib.empty() && key == "VIBIR") src = &m.vib[0].irIntensity;
    else if (!m.vib.empty() && key == "VIBRAM") src = &m.vib[0].ramanActivity;
    else if (const resultsh5::Trace *t = traceNamed(m, key)) {
      if (key == "SCANENERGY" || key == "IRCENERGY" || key == "TEVEC") {
        src = &t->energy;
        f = toHartree(v->units());
        f = f != 0 ? 1/f : 0;
      } else if (key == "SCANCOORD" || key == "IRCCOORD") {
        src = &t->value;
        f = valueFactor(t->valueUnit, v->units());
      }
    }
    if (src == 0 || src->size() != n || n == 0 || f == 0) return false;
    for (double x : *src) out.push_back(x*f);
    return true;
  }
  if (PropVecTable *v = dynamic_cast<PropVecTable*>(p)) {
    if (key != "VIB" || m.vib.empty() || v->columns() != 3 || (size_t)v->rows() != N)
      return false;
    if (m.vib[0].mode.size() != (size_t)v->tables()*3*N || v->tables() == 0) return false;
    out = m.vib[0].mode;
    return true;
  }
  if (PropTSVecTable *v = dynamic_cast<PropTSVecTable*>(p)) {
    const resultsh5::Trace *t = traceNamed(m, key);
    if (t == 0 || v->columns() != 3 || (size_t)v->rows() != N || v->tables() == 0 ||
        t->coord.size() != (size_t)v->tables()*3*N || toBohr(v->units()) == 0)
      return false;
    const double f = 1/toBohr(v->units());
    for (double x : t->coord) out.push_back(x*f);
    return true;
  }
  return false;
}

// Tables of `rows` x 3 from flat values, as setValues() takes them.
vector<vector<double> > *tablesOf(const vector<double>& flat, size_t rows)
{
  vector<vector<double> > *t = new vector<vector<double> >();
  for (size_t i = 0; i + 3*rows <= flat.size(); i += 3*rows)
    t->push_back(vector<double>(flat.begin() + i, flat.begin() + i + 3*rows));
  return t;
}

//  Views keep the pointers they are handed, so a copy is replaced only when
//  its values change, and the old one is kept, not freed.
struct Derived
{
  TProperty *source = 0;
  std::shared_ptr<const Model> model;
  long shape = -1;
  vector<double> values;
  std::unique_ptr<TProperty> prop;
};
std::map<string, Derived> g_derived;
vector<std::unique_ptr<TProperty> > g_retired;

// Values a property holds; a live update that grows it changes this.
long shapeOf(TProperty *p)
{
  if (PropVector *v = dynamic_cast<PropVector*>(p)) return v->rows();
  if (PropVecTable *v = dynamic_cast<PropVecTable*>(p)) return long(v->tables())*v->rows()*v->columns();
  if (PropTSVecTable *v = dynamic_cast<PropTSVecTable*>(p)) return long(v->tables())*v->rows()*v->columns();
  return -1;
}

}  // namespace


static TProperty *fileProperty(IPropCalculation *calc, const string& key)
{
  TProperty *p = calc ? calc->getProperty(key) : 0;
  if (p == 0) return 0;
  std::shared_ptr<const Model> m = storedAny(calc);
  if (!m) return p;
  //  Playback asks once per frame: an MD trace is not copied each time.
  Derived& d = g_derived[calc->getURL().toString() + "\n" + key];
  const long shape = shapeOf(p);
  if (d.source == p && d.model == m && d.shape == shape) return d.prop ? d.prop.get() : p;
  vector<double> vals;
  const bool fits = valuesFromFile(*m, key, p, vals);
  d.model = m;
  d.shape = shape;
  if (!fits) {
    if (d.prop) g_retired.push_back(std::move(d.prop));
    d.source = p;
    d.values.clear();
    return p;
  }
  if (d.prop && d.source == p && d.values == vals) return d.prop.get();
  TProperty *copy = 0;
  if (PropVector *v = dynamic_cast<PropVector*>(p)) {
    PropVector *c = new PropVector(*v);
    c->values(vals);
    copy = c;
  } else if (PropVecTable *v = dynamic_cast<PropVecTable*>(p)) {
    PropVecTable *c = new PropVecTable(*v);
    c->setValues(tablesOf(vals, v->rows()), v->rows(), 3);
    copy = c;
  } else if (PropTSVecTable *v = dynamic_cast<PropTSVecTable*>(p)) {
    PropTSVecTable *c = new PropTSVecTable(*v);
    c->setValues(tablesOf(vals, v->rows()), v->rows(), 3);
    copy = c;
  }
  if (copy == 0) return p;
  if (d.prop) g_retired.push_back(std::move(d.prop));
  d.prop.reset(copy);
  d.source = p;
  d.values.swap(vals);
  return copy;
}

namespace {

vector<string> numbered(size_t n)
{
  vector<string> l;
  for (size_t i = 0; i < n; i++) l.push_back(std::to_string(i + 1));
  return l;
}

// "1-C 2-H ...", the row labels the parsers give a structure table.
vector<string> atomLabels(const Model& m)
{
  TPerTab pt;
  vector<string> l;
  for (size_t a = 0; a < m.natoms(); a++)
    l.push_back(std::to_string(a + 1) + "-" + pt.atomicSymbol(m.atomicNumber[a]));
  return l;
}

const vector<double> *vibArray(const Model& m, const string& key)
{
  if (m.vib.empty()) return 0;
  const vector<double> *a = key == "VIBFREQ" ? &m.vib[0].frequency
      : key == "VIBIR" ? &m.vib[0].irIntensity
      : key == "VIBRAM" ? &m.vib[0].ramanActivity : 0;
  return a && !a->empty() ? a : 0;
}

// Scan or IRC coordinate unit back to the one the parsers write.
string coordUnit(const string& file, double& f)
{
  f = 1.0;
  if (file == "radian") { f = 180/3.14159265358979323846; return "Degree"; }
  if (file == "bohr") { f = 1/bohrPerAngstrom(); return "Angstrom"; }
  return file;
}

}  // namespace


// Layout as in the stored Props of every example (#883): VIBFREQ/IR/RAM
// rows "Normal Mode" 1..n; VIB rows = atoms 1..N, one "NA"-labelled table
// per mode; structure tables "Atom" X Y Z with "i-El" rows; energies in
// Hartree and structures in Angstrom.  Gaussian's mode symmetry labels
// ("1-A1") are not in the file, so its modes come back numbered.
bool fromFile(IPropCalculation *calc, const string& key, TProperty *p)
{
  if (calc == 0 || p == 0) return false;
  std::shared_ptr<const Model> sm = storedAny(calc);
  if (!sm) return false;
  const Model& m = *sm;
  const size_t N = m.natoms();
  const resultsh5::Trace *t = traceNamed(m, key);
  const string point = key.compare(0, 3, "IRC") == 0 ? "IRC" : "Scan";

  if (const vector<double> *a = vibArray(m, key)) {
    PropVector *v = dynamic_cast<PropVector*>(p);
    if (v == 0) return false;
    v->rowLabel("Normal Mode");
    v->units(key == "VIBFREQ" ? "cm-1" : key == "VIBIR" ? "KM/Mole" : "A**4/AMU");
    vector<string> l = numbered(a->size());
    v->rowLabels(&l);
    v->values(*a);
    return true;
  }
  if (key == "VIB") {
    PropVecTable *v = dynamic_cast<PropVecTable*>(p);
    if (v == 0 || m.vib.empty() || N == 0) return false;
    const vector<double>& mode = m.vib[0].mode;
    const size_t nm = mode.size()/(3*N);
    if (nm == 0 || mode.size() != nm*3*N) return false;
    v->rowLabel("Normal Mode");
    v->columnLabel("Atom");
    v->units("NA");
    vector<string> rows = numbered(N), cols = {"X", "Y", "Z"}, tabs(nm, "NA");
    v->labels(&rows, &cols, &tabs);
    v->setValues(tablesOf(mode, N), N, 3);
    return true;
  }
  if (t == 0 || N == 0) return false;
  const size_t np = t->npoint(N);
  if (np == 0 || t->coord.size() != np*3*N) return false;

  if (key == "SCANGEOM" || key == "IRCGEOM" || key == "GEOMTRACE") {
    PropTSVecTable *v = dynamic_cast<PropTSVecTable*>(p);
    if (v == 0) return false;
    v->rowLabel(key == "GEOMTRACE" ? "Geometry Step" : point + " Step");
    v->columnLabel("Atom");
    v->units("Angstrom");
    vector<string> rows = atomLabels(m), cols = {"X", "Y", "Z"}, tabs;
    v->labels(&rows, &cols, &tabs);
    vector<double> x(t->coord);
    for (double& c : x) c /= bohrPerAngstrom();
    v->setValues(tablesOf(x, N), N, 3);
    return true;
  }
  if (key == "TEVEC") {
    PropTSVector *v = dynamic_cast<PropTSVector*>(p);
    if (v == 0 || t->energy.size() != np) return false;
    v->rowLabel("Geometry Step");
    v->units("Hartree");
    v->insertValues(t->energy);
    return true;
  }
  PropVector *v = dynamic_cast<PropVector*>(p);
  if (v == 0) return false;
  if (key == "SCANENERGY" || key == "IRCENERGY") {
    if (t->energy.size() != np) return false;
    v->rowLabel(point + " Point");
    v->units("Hartree");
    v->values(t->energy);
    return true;
  }
  if (key == "SCANCOORD" || key == "IRCCOORD") {
    if (t->value.size() != np) return false;
    double f = 1;
    v->rowLabel(point + " Point");
    v->units(coordUnit(t->valueUnit, f));
    vector<double> x(t->value);
    for (double& c : x) c *= f;
    v->values(x);
    return true;
  }
  return false;
}


vector<string> keysInFile(IPropCalculation *calc)
{
  vector<string> keys;
  std::shared_ptr<const Model> sm = storedAny(calc);
  if (!sm) return keys;
  const Model& m = *sm;
  const size_t N = m.natoms();
  for (const char *k : {"VIBFREQ", "VIBIR", "VIBRAM"})
    if (vibArray(m, k)) keys.push_back(k);
  if (!m.vib.empty() && N > 0 && !m.vib[0].mode.empty()) keys.push_back("VIB");
  for (const char *k : {"SCANGEOM", "SCANENERGY", "SCANCOORD", "IRCGEOM",
                        "IRCENERGY", "IRCCOORD", "GEOMTRACE", "TEVEC"}) {
    const resultsh5::Trace *t = traceNamed(m, k);
    if (t == 0 || t->npoint(N) == 0) continue;
    const string key = k;
    const size_t np = t->npoint(N);
    if (key.find("ENERGY") != string::npos || key == "TEVEC") {
      if (t->energy.size() == np) keys.push_back(key);
    } else if (key.find("COORD") != string::npos) {
      if (t->value.size() == np) keys.push_back(key);
    } else if (key != "GEOMTRACE" || np > 1) keys.push_back(key);
  }
  return keys;
}

#else   // no HDF5: no results.h5

bool convert(const Parsed&, resultsh5::Model&, string& why) { why = "built without HDF5"; return false; }
bool gather(IPropCalculation*, Parsed&, string& why) { why = "built without HDF5"; return false; }
bool codeColumns(IPropCalculation*, const resultsh5::Model&, vector<vector<double> >*, string& why) { why = "built without HDF5"; return false; }
void addProperties(IPropCalculation*, resultsh5::Model&) {}
string check(resultsh5::Model&) { return string(); }
bool build(IPropCalculation*, resultsh5::Model&, string&, string& why) { why = "built without HDF5"; return false; }
bool store(IPropCalculation*, string& line) { line = "built without HDF5"; return false; }
std::shared_ptr<const resultsh5::Model> stored(IPropCalculation*) { return std::shared_ptr<const resultsh5::Model>(); }
std::shared_ptr<const resultsh5::Model> storedAny(IPropCalculation*) { return std::shared_ptr<const resultsh5::Model>(); }
static TProperty *fileProperty(IPropCalculation *calc, const string& key) { return calc ? calc->getProperty(key) : 0; }
void forget(IPropCalculation*) {}
bool fromFile(IPropCalculation*, const string&, TProperty*) { return false; }
vector<string> keysInFile(IPropCalculation*) { return vector<string>(); }

#endif

namespace {

struct IrcView
{
  TProperty *src[3] = {0, 0, 0};
  long frames = -1;
  vector<double> coord, energy;
  std::unique_ptr<TProperty> prop;
};
std::map<string, IrcView> g_irc;
vector<std::unique_ptr<TProperty> > g_ircRetired;

}  // namespace

TProperty *property(IPropCalculation *calc, const string& key)
{
  TProperty *p = fileProperty(calc, key);
  if (p == 0 || (key != "IRCGEOM" && key != "IRCENERGY" && key != "IRCCOORD")) return p;
  PropVector *c = dynamic_cast<PropVector*>(fileProperty(calc, "IRCCOORD"));
  PropVector *e = dynamic_cast<PropVector*>(fileProperty(calc, "IRCENERGY"));
  PropTSVecTable *g = dynamic_cast<PropTSVecTable*>(fileProperty(calc, "IRCGEOM"));
  //  Views keep the pointers they get: a copy is made again only when one of
  //  the three changed (the summary replaces the live values) or grew, and
  //  the old one is retired, not freed.
  IrcView& v = g_irc[calc->getURL().toString() + "\n" + key];
  TProperty *src[3] = {c, e, g};
  const long frames = g ? (long)g->tables() : -1;
  const vector<double> coord = c ? c->values() : vector<double>();
  const vector<double> energy = e ? e->values() : vector<double>();
  if (std::equal(src, src + 3, v.src) && frames == v.frames && coord == v.coord &&
      energy == v.energy)
    return v.prop ? v.prop.get() : p;
  std::copy(src, src + 3, v.src);
  v.frames = frames;
  v.coord = coord;
  v.energy = energy;
  if (v.prop) g_ircRetired.push_back(std::move(v.prop));
  IrcPath path;
  if (!ircPath(c, e, g, path)) return p;
  v.prop.reset(ircCopy(path, key));
  return v.prop.get();
}

}
