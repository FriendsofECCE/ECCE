// Fragment-orbital analysis from results.h5 models (#201); see
// include/tdat/FragmentAnalysis.H for the formulas.
#include "tdat/FragmentAnalysis.H"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <set>

namespace fragorb {

using namespace resultsh5;

namespace {

const double BOHR_ANG = 0.529177210903;
const double HARTREE_EV = 27.211386245988;

string fmt(const char* f, ...) __attribute__((format(printf, 1, 2)));
string fmt(const char* f, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, f);
  vsnprintf(buf, sizeof buf, f, ap);
  va_end(ap);
  return buf;
}

size_t shellSize(const Shell& s) {
  return s.cartesian ? size_t((s.l + 1) * (s.l + 2) / 2) : size_t(2 * s.l + 1);
}

// AO index ranges (start, count) of each shell, canonical atom-major order.
struct AtomShells {
  vector<size_t> first;       // first AO of each shell
  vector<const Shell*> shell;
};
vector<AtomShells> shellsByAtom(const Model& m) {
  vector<AtomShells> r(m.natoms());
  size_t ao = 0;
  for (size_t k = 0; k < m.basis.shells.size(); k++) {
    const Shell& s = m.basis.shells[k];
    if (s.atom >= 0 && size_t(s.atom) < r.size()) {
      r[s.atom].first.push_back(ao);
      r[s.atom].shell.push_back(&s);
    }
    ao += shellSize(s);
  }
  return r;
}

bool sameShell(const Shell& a, const Shell& b) {
  if (a.l != b.l || a.cartesian != b.cartesian || a.exponent.size() != b.exponent.size()) return false;
  double ca = 0, cb = 0;
  for (size_t i = 0; i < a.coef.size(); i++) ca = std::max(ca, std::fabs(a.coef[i]));
  for (size_t i = 0; i < b.coef.size(); i++) cb = std::max(cb, std::fabs(b.coef[i]));
  if (a.coef.size() != b.coef.size() || ca == 0 || cb == 0) return false;
  for (size_t i = 0; i < a.exponent.size(); i++) {
    if (std::fabs(a.exponent[i] - b.exponent[i]) > 1e-6 * std::fabs(a.exponent[i])) return false;
    if (std::fabs(a.coef[i] / ca - b.coef[i] / cb) > 1e-5) return false;
  }
  return true;
}

// C = A (m x k) * B (k x n), row-major.
void matmul(const vector<double>& A, const vector<double>& B, size_t m, size_t k, size_t n, vector<double>& C) {
  C.assign(m * n, 0.0);
  for (size_t i = 0; i < m; i++)
    for (size_t p = 0; p < k; p++) {
      double a = A[i * k + p];
      if (a == 0) continue;
      const double* b = &B[p * n];
      double* c = &C[i * n];
      for (size_t j = 0; j < n; j++) c[j] += a * b[j];
    }
}

// In-place Cholesky solve of A X = B (A n x n SPD, B n x r); false if A is
// not positive definite (linearly dependent FOs).
bool cholSolve(vector<double> A, size_t n, vector<double>& B, size_t r, double& minPivot) {
  minPivot = 1e300;
  for (size_t j = 0; j < n; j++) {
    double d = A[j * n + j];
    for (size_t k = 0; k < j; k++) d -= A[j * n + k] * A[j * n + k];
    if (d < minPivot) minPivot = d;
    if (d <= 1e-10) return false;
    d = std::sqrt(d);
    A[j * n + j] = d;
    for (size_t i = j + 1; i < n; i++) {
      double s = A[i * n + j];
      for (size_t k = 0; k < j; k++) s -= A[i * n + k] * A[j * n + k];
      A[i * n + j] = s / d;
    }
  }
  for (size_t c = 0; c < r; c++) {
    for (size_t i = 0; i < n; i++) {
      double s = B[i * r + c];
      for (size_t k = 0; k < i; k++) s -= A[i * n + k] * B[k * r + c];
      B[i * r + c] = s / A[i * n + i];
    }
    for (size_t ii = n; ii-- > 0;) {
      double s = B[ii * r + c];
      for (size_t k = ii + 1; k < n; k++) s -= A[k * n + ii] * B[k * r + c];
      B[ii * r + c] = s / A[ii * n + ii];
    }
  }
  return true;
}

const Mos* mosFor(const Model& m, int spin) {
  for (size_t k = 0; k < m.mos.size(); k++) if (m.mos[k].spin == spin) return &m.mos[k];
  return 0;
}

// Complex AO index of every AO of a fragment, atom by atom; checks the bases.
bool aoMap(const Model& cx, const Model& fr, const vector<int>& atomMap,
           const vector<AtomShells>& cs, const vector<AtomShells>& fs,
           const string& name, vector<size_t>& idx, string& err) {
  idx.assign(fr.basis.nao(), size_t(-1));
  for (size_t a = 0; a < fr.natoms(); a++) {
    const AtomShells& f = fs[a];
    const AtomShells& c = cs[atomMap[a]];
    bool ok = f.shell.size() == c.shell.size();
    for (size_t s = 0; ok && s < f.shell.size(); s++) ok = sameShell(*f.shell[s], *c.shell[s]);
    if (!ok) {
      err = fmt("fragment %s: the basis on atom %zu (%d) differs from the complex's basis on atom %d; "
                "run the fragment with the complex's basis on its atoms",
                name.c_str(), a + 1, fr.atomicNumber[a], atomMap[a] + 1);
      return false;
    }
    for (size_t s = 0; s < f.shell.size(); s++)
      for (size_t i = 0; i < shellSize(*f.shell[s]); i++) idx[f.first[s] + i] = c.first[s] + i;
  }
  for (size_t i = 0; i < idx.size(); i++)
    if (idx[i] == size_t(-1)) { err = "fragment " + name + ": a basis function belongs to no atom"; return false; }
  return true;
}

int geometryOf(const Model& m) { return m.mos.empty() ? 0 : m.mos[0].geometry; }

}  // namespace

bool mapAtoms(const Model& cx, const Model& fr, double tolAng, vector<int>& map, string& err) {
  int gc = geometryOf(cx), gf = geometryOf(fr);
  size_t nc = cx.natoms(), nf = fr.natoms();
  if (size_t(gc) >= cx.nframes() || size_t(gf) >= fr.nframes()) { err = "geometry frame missing"; return false; }
  map.assign(nf, -1);
  std::set<int> used;
  for (size_t a = 0; a < nf; a++) {
    int best = -1;
    double bd = 1e30;
    for (size_t c = 0; c < nc; c++) {
      if (cx.atomicNumber[c] != fr.atomicNumber[a]) continue;
      double d2 = 0;
      for (int x = 0; x < 3; x++) {
        double d = (cx.coord[(size_t(gc) * nc + c) * 3 + x] - fr.coord[(size_t(gf) * nf + a) * 3 + x]) * BOHR_ANG;
        d2 += d * d;
      }
      if (d2 < bd) { bd = d2; best = int(c); }
    }
    if (best < 0 || std::sqrt(bd) > tolAng) {
      err = fmt("fragment atom %zu (Z=%d) matches no complex atom within %g Angstrom (nearest same-element distance %.4f)",
                a + 1, fr.atomicNumber[a], tolAng, best < 0 ? -1.0 : std::sqrt(bd));
      return false;
    }
    if (!used.insert(best).second) { err = fmt("fragment atoms %zu and another both match complex atom %d", a + 1, best + 1); return false; }
    map[a] = best;
  }
  return true;
}

bool analyse(const Model& cx, const vector<NamedModel>& frags, const Options& opt, Analysis& out, string& err) {
  out = Analysis();
  if (frags.empty()) { err = "no fragments"; return false; }
  if (cx.basis.type != "GTO" || cx.basis.orthonormal) { err = "the complex has no Gaussian basis (semiempirical or plane-wave results cannot be analysed)"; return false; }
  if (cx.mos.empty()) { err = "the complex has no molecular orbitals"; return false; }
  vector<AtomShells> cs = shellsByAtom(cx);
  size_t nao = cx.basis.nao();
  vector<double> S;
  if (!overlapMatrix(cx, geometryOf(cx), S, err)) { err = "complex overlap: " + err; return false; }

  size_t nf = frags.size();
  vector<vector<size_t> > aos(nf);
  std::map<int,int> owner;
  for (size_t f = 0; f < nf; f++) {
    const Model& fr = *frags[f].model;
    const string& nm = frags[f].name;
    out.fragmentNames.push_back(nm);
    if (fr.basis.type != "GTO" || fr.basis.orthonormal || fr.mos.empty()) { err = "fragment " + nm + " has no Gaussian-basis orbitals"; return false; }
    out.atomMap.push_back(vector<int>());
    if (!mapAtoms(cx, fr, opt.atomTolAng, out.atomMap.back(), err)) { err = "fragment " + nm + ": " + err; return false; }
    for (size_t a = 0; a < out.atomMap.back().size(); a++) {
      int c = out.atomMap.back()[a];
      if (owner.count(c)) {
        err = fmt("fragments %s and %s overlap: complex atom %d is in both", frags[owner[c]].name.c_str(), nm.c_str(), c + 1);
        return false;
      }
      owner[c] = int(f);
    }
    if (!aoMap(cx, fr, out.atomMap.back(), cs, shellsByAtom(fr), nm, aos[f], err)) return false;
  }
  if (owner.size() != cx.natoms() && !opt.allowUncovered) {
    string list;
    for (size_t c = 0; c < cx.natoms(); c++) if (!owner.count(int(c))) list += fmt(" %zu", c + 1);
    err = "the fragments do not cover the complex; uncovered atoms:" + list + " (--allow-uncovered to analyse anyway)";
    return false;
  }

  bool cxUnres = mosFor(cx, 1) != 0;
  out.restricted = !cxUnres;
  for (int spin = 0; spin < (cxUnres ? 2 : 1); spin++) {
    const Mos* cm = mosFor(cx, spin);
    if (!cm) { err = "the complex lacks a spin set"; return false; }
    if (cm->nao != nao) { err = "complex MO and basis sizes differ"; return false; }
    Channel ch;
    ch.spin = spin;
    ch.nmo = cm->nmo;
    ch.energy = cm->energy;
    ch.occupation = cm->occupation;
    vector<const Mos*> fm(nf);
    size_t nfo = 0;
    vector<size_t> off(nf + 1, 0);
    for (size_t f = 0; f < nf; f++) {
      const Model& fr = *frags[f].model;
      fm[f] = mosFor(fr, spin);
      if (!fm[f] && spin == 0) fm[f] = &fr.mos[0];
      if (!fm[f] && spin == 1) fm[f] = mosFor(fr, 0);   // restricted fragment serves both spins
      if (cxUnres == false && mosFor(fr, 1)) {
        err = "fragment " + frags[f].name + " is unrestricted but the complex is restricted; "
              "an open-shell fragment of a closed-shell complex needs a restricted (ROHF/ROKS) fragment calculation";
        return false;
      }
      if (fm[f]->nao != aos[f].size()) { err = "fragment " + frags[f].name + ": MO and basis sizes differ"; return false; }
      off[f] = nfo;
      for (size_t i = 0; i < fm[f]->nmo; i++) {
        Fo fo;
        fo.fragment = int(f);
        fo.index = int(i + 1);
        fo.energy = i < fm[f]->energy.size() ? fm[f]->energy[i] : 0;
        fo.occupation = i < fm[f]->occupation.size() ? fm[f]->occupation[i] : 0;
        ch.fo.push_back(fo);
      }
      nfo += fm[f]->nmo;
    }
    off[nf] = nfo;
    size_t nmo = cm->nmo;
    // c^T: nao x nmo
    vector<double> cT(nao * nmo);
    for (size_t i = 0; i < nmo; i++) for (size_t p = 0; p < nao; p++) cT[p * nmo + i] = cm->coef[i * nao + p];

    // Fragment coefficients embedded as rows over the fragment's own AOs,
    // gathered S rows: B = C_f (S_f,all c^T); S_FO blocks via S(f,g).
    vector<double> B(nfo * nmo, 0.0), SFO(nfo * nfo, 0.0);
    for (size_t f = 0; f < nf; f++) {
      size_t na = aos[f].size(), nm = fm[f]->nmo;
      vector<double> Srows(na * nao);
      for (size_t i = 0; i < na; i++) for (size_t p = 0; p < nao; p++) Srows[i * nao + p] = S[aos[f][i] * nao + p];
      vector<double> G, Bf;
      matmul(Srows, cT, na, nao, nmo, G);
      matmul(fm[f]->coef, G, nm, na, nmo, Bf);
      std::copy(Bf.begin(), Bf.end(), B.begin() + off[f] * nmo);
      for (size_t g = f; g < nf; g++) {
        size_t nb = aos[g].size(), ng = fm[g]->nmo;
        vector<double> Sfg(na * nb), T, Blk;
        for (size_t i = 0; i < na; i++) for (size_t j = 0; j < nb; j++) Sfg[i * nb + j] = S[aos[f][i] * nao + aos[g][j]];
        vector<double> CgT(nb * ng);
        for (size_t i = 0; i < ng; i++) for (size_t j = 0; j < nb; j++) CgT[j * ng + i] = fm[g]->coef[i * nb + j];
        matmul(Sfg, CgT, na, nb, ng, T);
        matmul(fm[f]->coef, T, nm, na, ng, Blk);
        for (size_t i = 0; i < nm; i++)
          for (size_t j = 0; j < ng; j++) {
            SFO[(off[f] + i) * nfo + off[g] + j] = Blk[i * ng + j];
            SFO[(off[g] + j) * nfo + off[f] + i] = Blk[i * ng + j];
          }
      }
    }
    ch.coef = B;
    double minPivot;
    if (!cholSolve(SFO, nfo, ch.coef, nmo, minPivot)) {
      err = fmt("the fragment orbitals are linearly dependent (overlap pivot %.2e): "
                "fragment bases overlap too much, or a fragment holds duplicate orbitals", minPivot);
      return false;
    }
    ch.mulliken.assign(nfo * nmo, 0.0);
    ch.scpa.assign(nfo * nmo, 0.0);
    ch.remainder.assign(nmo, 1.0);
    for (size_t i = 0; i < nmo; i++) {
      double sq = 0, cap = 0;
      for (size_t k = 0; k < nfo; k++) {
        double a = ch.coef[k * nmo + i];
        sq += a * a;
        ch.mulliken[k * nmo + i] = a * B[k * nmo + i];
        cap += a * B[k * nmo + i];
      }
      for (size_t k = 0; k < nfo; k++) {
        double a = ch.coef[k * nmo + i];
        ch.scpa[k * nmo + i] = sq > 0 ? a * a / sq : 0;
      }
      ch.remainder[i] = 1.0 - cap;
    }
    out.channel.push_back(ch);
  }
  return true;
}

vector<Connection> connections(const Analysis& a, double threshold, const string& method) {
  vector<Connection> r;
  for (size_t c = 0; c < a.channel.size(); c++) {
    const Channel& ch = a.channel[c];
    const vector<double>& comp = method == "scpa" ? ch.scpa : ch.mulliken;
    for (size_t k = 0; k < ch.fo.size(); k++)
      for (size_t i = 0; i < ch.nmo; i++) {
        double v = comp[k * ch.nmo + i];
        if (v >= threshold) {
          Connection cn = { ch.spin, int(i), int(k), v };
          r.push_back(cn);
        }
      }
  }
  return r;
}

bool dos(const Model& cx, const DosOptions& opt, DosResult& out, string& err) {
  out = DosResult();
  if (cx.basis.type != "GTO" || cx.basis.orthonormal || cx.mos.empty()) { err = "no Gaussian-basis orbitals"; return false; }
  if (opt.sets.empty()) { err = "no atom sets"; return false; }
  size_t nao = cx.basis.nao(), nset = opt.sets.size();
  vector<double> S;
  if (!overlapMatrix(cx, geometryOf(cx), S, err)) return false;
  vector<AtomShells> cs = shellsByAtom(cx);
  vector<int> setOf(nao, -1);
  for (size_t s = 0; s < nset; s++) {
    out.setNames.push_back(opt.sets[s].name);
    for (size_t k = 0; k < opt.sets[s].atoms.size(); k++) {
      int at = opt.sets[s].atoms[k];
      if (at < 0 || size_t(at) >= cx.natoms()) { err = "atom index out of range in set " + opt.sets[s].name; return false; }
      for (size_t q = 0; q < cs[at].shell.size(); q++)
        for (size_t i = 0; i < shellSize(*cs[at].shell[q]); i++) {
          int& o = setOf[cs[at].first[q] + i];
          if (o >= 0 && o != int(s)) { err = fmt("atom %d is in sets %s and %s", at + 1, opt.sets[o].name.c_str(), opt.sets[s].name.c_str()); return false; }
          o = int(s);
        }
    }
  }
  vector<std::pair<int,int> > pairs = opt.pairs;
  if (pairs.empty())
    for (size_t a = 0; a < nset; a++) for (size_t b = a + 1; b < nset; b++) pairs.push_back(std::make_pair(int(a), int(b)));
  size_t np = pairs.size();
  for (size_t p = 0; p < np; p++) out.pairNames.push_back(opt.sets[pairs[p].first].name + "-" + opt.sets[pairs[p].second].name);

  bool unres = mosFor(cx, 1) != 0;
  for (int spin = 0; spin < (unres ? 2 : 1); spin++) {
    const Mos* m = mosFor(cx, spin);
    if (!m) { err = "missing spin set"; return false; }
    for (size_t i = 0; i < m->nmo; i++) {
      const double* c = &m->coef[i * nao];
      vector<double> Sc(nao, 0.0);
      for (size_t p = 0; p < nao; p++) {
        double s = 0;
        for (size_t q = 0; q < nao; q++) s += S[p * nao + q] * c[q];
        Sc[p] = s;
      }
      vector<double> gross(nset, 0.0), ov(np, 0.0);
      // cross[a][b] = sum over mu in a, nu in b of c_mu c_nu S_mu,nu
      vector<double> cross(nset * nset, 0.0);
      for (size_t p = 0; p < nao; p++) {
        if (setOf[p] < 0) continue;
        for (size_t q = 0; q < nao; q++)
          if (setOf[q] >= 0) cross[setOf[p] * nset + setOf[q]] += c[p] * c[q] * S[p * nao + q];
      }
      // Mulliken gross population: own block plus half of each overlap
      for (size_t p = 0; p < nao; p++) if (setOf[p] >= 0) gross[setOf[p]] += c[p] * Sc[p];
      for (size_t k = 0; k < np; k++) ov[k] = 2.0 * cross[pairs[k].first * nset + pairs[k].second];
      out.moEnergy.push_back((i < m->energy.size() ? m->energy[i] : 0) * HARTREE_EV);
      out.moOccupation.push_back(i < m->occupation.size() ? m->occupation[i] : 0);
      out.moWeight.push_back(unres ? 1.0 : 2.0);
      out.moGross.insert(out.moGross.end(), gross.begin(), gross.end());
      out.moOverlap.insert(out.moOverlap.end(), ov.begin(), ov.end());
    }
  }
  size_t nm = out.moEnergy.size();
  // Fermi level: midpoint of the highest occupied and lowest empty MO
  double homo = -1e30, lumo = 1e30;
  for (size_t i = 0; i < nm; i++) {
    if (out.moOccupation[i] > 0.5 * (unres ? 1.0 : 2.0) - 1e-6) homo = std::max(homo, out.moEnergy[i]);
    else lumo = std::min(lumo, out.moEnergy[i]);
  }
  if (homo < -1e29 || lumo > 1e29) out.fermiEv = homo < -1e29 ? lumo : homo;
  else out.fermiEv = 0.5 * (homo + lumo);
  double e0 = opt.emin, e1 = opt.emax;
  if (e0 == e1) { e0 = (homo < -1e29 ? out.moEnergy[0] : homo) - 20; e1 = (lumo > 1e29 ? out.moEnergy.back() : lumo) + 10; }
  double step = opt.step > 0 ? opt.step : 0.05;
  size_t npts = size_t(std::floor((e1 - e0) / step + 0.5)) + 1;
  out.energy.resize(npts);
  out.tdos.assign(npts, 0.0);
  out.pdos.assign(npts * nset, 0.0);
  out.opdos.assign(npts * np, 0.0);
  out.intOpdos.assign(npts * np, 0.0);
  out.occupiedOverlap.assign(np, 0.0);
  double sigma = opt.fwhmEv / (2.0 * std::sqrt(2.0 * std::log(2.0)));
  double hw = 0.5 * opt.fwhmEv;
  for (size_t g = 0; g < npts; g++) {
    double e = e0 + step * double(g);
    out.energy[g] = e;
    for (size_t i = 0; i < nm; i++) {
      double d = e - out.moEnergy[i], f;
      if (opt.lorentzian) f = hw / M_PI / (d * d + hw * hw);
      else f = std::exp(-0.5 * d * d / (sigma * sigma)) / (sigma * std::sqrt(2.0 * M_PI));
      f *= out.moWeight[i];
      if (f < 1e-14) continue;
      out.tdos[g] += f;
      for (size_t s = 0; s < nset; s++) out.pdos[g * nset + s] += f * out.moGross[i * nset + s];
      for (size_t k = 0; k < np; k++) out.opdos[g * np + k] += f * out.moOverlap[i * np + k];
    }
  }
  for (size_t k = 0; k < np; k++) {
    double acc = 0;
    for (size_t g = 0; g < npts; g++) {
      acc += out.opdos[g * np + k] * step;
      out.intOpdos[g * np + k] = acc;
    }
    for (size_t i = 0; i < nm; i++) out.occupiedOverlap[k] += out.moOccupation[i] * out.moOverlap[i * np + k];
  }
  return true;
}

}
