// Fragment-orbital analysis from results.h5 models (#201); see
// include/tdat/FragmentAnalysis.H for the formulas.
#include "tdat/FragmentAnalysis.H"

#include <algorithm>
#include <cctype>
#include <map>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <set>
#include <sstream>

#include "tdat/CharacterTable.H"
#include "tdat/Fragment.H"
#include "tdat/MoFragments.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/SymmetryOps.H"
#include "tdat/TAtm.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/TPerTab.H"

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
    out.fragmentModels.push_back(frags[f].model);
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

FieldCheck fieldCheck(const Analysis& a, double dMin, double occMin) {
  FieldCheck r;
  // The fragment is a lone atom with a d set: at least five of its orbitals
  // are mostly d (Mulliken population of l = 2 of 50 % or more).
  for (size_t f = 0; f < a.fragmentModels.size() && !r.applicable; f++) {
    const Model* fm = a.fragmentModels[f];
    if (!fm || fm->natoms() != 1 || a.channel.empty()) continue;
    const Mos* fmo = mosFor(*fm, 0);
    if (!fmo) fmo = &fm->mos[0];
    vector<double> Sf;
    string err;
    if (!overlapMatrix(*fm, geometryOf(*fm), Sf, err)) continue;
    const size_t na = fmo->nao;
    vector<int> lOf;
    for (size_t k = 0; k < fm->basis.shells.size(); k++)
      for (size_t i = 0; i < shellSize(fm->basis.shells[k]); i++) lOf.push_back(fm->basis.shells[k].l);
    if (lOf.size() != na) continue;
    vector<char> isD(fmo->nmo, 0);
    size_t nd = 0;
    for (size_t i = 0; i < fmo->nmo; i++) {
      const double* c = &fmo->coef[i * na];
      double pop = 0;
      for (size_t mu = 0; mu < na; mu++) {
        if (lOf[mu] != 2) continue;
        double sc = 0;
        for (size_t nu = 0; nu < na; nu++) sc += Sf[mu * na + nu] * c[nu];
        pop += c[mu] * sc;
      }
      if (pop >= 0.5) { isD[i] = 1; nd++; }
    }
    if (nd < 5) continue;
    r.applicable = true;
    r.fragment = int(f);
    for (const Channel& ch : a.channel)
      for (size_t i = 0; i < ch.nmo; i++) {
        if (ch.occupation[i] <= 0.5) continue;
        double d = 0, occ = 0;
        for (size_t k = 0; k < ch.fo.size(); k++) {
          if (ch.fo[k].fragment != int(f) || size_t(ch.fo[k].index) > isD.size() || !isD[ch.fo[k].index - 1]) continue;
          const double m = ch.mulliken[k * ch.nmo + i];
          d += m;
          if (ch.fo[k].occupation > 0.5) occ += m;
        }
        if (d < dMin) continue;
        r.checked++;
        if (occ / d < r.worst) { r.worst = occ / d; r.mo = int(i + 1); r.dShare = d; }
      }
    r.ok = r.checked == 0 || r.worst >= occMin;
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

namespace {

struct Run { int first, last; };

// "MO 52-54" with an en dash
string levelName(const char* what, const vector<int>& n) {
  if (n.size() == 1) return fmt("%s %d", what, n[0]);
  return fmt("%s %d\xE2\x80\x93%d", what, n.front(), n.back());
}

// HOMO/LUMO position of orbitals lo..hi (1-based) of a set whose HOMO is h.
string position(int h, int lo, int hi) {
  if (h >= lo && h <= hi) return "HOMO";
  if (hi < h) return fmt("HOMO\xE2\x88\x92%d", h - hi);
  if (lo == h + 1) return "LUMO";
  return fmt("LUMO+%d", lo - h - 1);
}

}  // namespace

bool diagram(const Model& cx, const Analysis& a, const DiagramOptions& opt, Diagram& out, string& err) {
  out = Diagram();
  out.fragmentNames = a.fragmentNames;
  out.options = opt;
  const size_t nf = a.fragmentNames.size();
  if (nf == 0 || a.channel.empty()) { err = "no analysis"; return false; }
  if (opt.threshold <= 0 || opt.threshold > 1) { err = "the threshold must be between 0 and 1"; return false; }

  // Overlap population of every MO between the fragments' atoms (complex S).
  DosResult dr;
  vector<vector<int> > pairIndex(nf, vector<int>(nf, -1));
  if (nf >= 2) {
    DosOptions dopt;
    for (size_t f = 0; f < nf; f++) {
      AtomSet as;
      as.name = a.fragmentNames[f];
      as.atoms = a.atomMap[f];
      dopt.sets.push_back(as);
    }
    dopt.emin = -1;
    dopt.emax = 1;
    dopt.step = 1;
    if (!dos(cx, dopt, dr, err)) { err = "overlap populations: " + err; return false; }
    int p = 0;
    for (size_t f = 0; f < nf; f++)
      for (size_t g = f + 1; g < nf; g++) pairIndex[f][g] = pairIndex[g][f] = p++;
  }
  const size_t np = dr.pairNames.size();

  auto lower = [](string x) { if (!x.empty()) x[0] = char(std::tolower((unsigned char)x[0])); return x; };
  size_t moBase = 0;
  for (size_t c = 0; c < a.channel.size(); c++) {
    const Channel& ch = a.channel[c];
    // Irreps of the complex's and each fragment's own orbitals.
    OrbitalSymmetry csym;
    vector<OrbitalSymmetry> fsym(nf);
    const bool cok = opt.symmetry && orbitalSymmetry(cx, ch.spin, csym);
    vector<char> fok(nf, 0);
    for (size_t f = 0; f < nf; f++)
      fok[f] = opt.symmetry && f < a.fragmentModels.size() && a.fragmentModels[f] &&
               orbitalSymmetry(*a.fragmentModels[f], ch.spin, fsym[f]);
    if (c == 0) {
      out.groups.assign(1, cok ? csym.group : "");
      out.symmetryWhy.assign(1, cok ? "" : (opt.symmetry ? csym.why : "not asked for"));
      for (size_t f = 0; f < nf; f++) {
        out.groups.push_back(fok[f] ? fsym[f].group : "");
        out.symmetryWhy.push_back(fok[f] ? "" : (opt.symmetry ? fsym[f].why : "not asked for"));
      }
    }
    const vector<double>& comp = opt.method == "scpa" ? ch.scpa : ch.mulliken;
    const size_t nmo = ch.nmo, nfo = ch.fo.size();
    DiagramChannel dc;
    dc.spin = ch.spin;

    vector<Run> ml, fl;
    for (size_t i = 0; i < nmo;) {
      size_t j = i;
      while (j + 1 < nmo && std::fabs(ch.energy[j + 1] - ch.energy[i]) < opt.degenerateEh) j++;
      Run r = { int(i), int(j) };
      ml.push_back(r);
      i = j + 1;
    }
    for (size_t k = 0; k < nfo;) {
      size_t j = k;
      while (j + 1 < nfo && ch.fo[j + 1].fragment == ch.fo[k].fragment &&
             std::fabs(ch.fo[j + 1].energy - ch.fo[k].energy) < opt.degenerateEh) j++;
      Run r = { int(k), int(j) };
      fl.push_back(r);
      k = j + 1;
    }
    const size_t nml = ml.size(), nfl = fl.size();
    // "2t2g": the irrep shared by every orbital of the level, counted from
    // the lowest level of that irrep; "" when the orbitals disagree or
    // have no label.
    vector<string> mlIrrep(nml), flIrrep(nfl), flShell(nfl);
    // A one-atom fragment has no table group: its FO levels are named by
    // the shell they are made of (Mulliken population of one l >= 90 %),
    // n counted from the orbitals of that l below them (model choice: the
    // aufbau count, so a split d set stays 3d).
    for (size_t f = 0; f < nf; f++) {
      const Model* fm = f < a.fragmentModels.size() ? a.fragmentModels[f] : 0;
      if (!fm || fm->natoms() != 1) continue;
      const Mos* fmo = mosFor(*fm, ch.spin);
      if (!fmo) fmo = &fm->mos[0];
      vector<double> Sf;
      string serr;
      if (!overlapMatrix(*fm, geometryOf(*fm), Sf, serr)) continue;
      const size_t na = fmo->nao;
      vector<int> lOf;
      for (size_t k = 0; k < fm->basis.shells.size(); k++)
        for (size_t i = 0; i < shellSize(fm->basis.shells[k]); i++) lOf.push_back(fm->basis.shells[k].l);
      if (lOf.size() != na) continue;
      const string el = TPerTab().atomicSymbol(fm->atomicNumber[0]);
      vector<int> lBelow(8, 0);
      // FO levels of this fragment, lowest first (fl is in FO order, as stored)
      vector<size_t> ks;
      for (size_t k = 0; k < nfl; k++) if (ch.fo[fl[k].first].fragment == int(f)) ks.push_back(k);
      std::stable_sort(ks.begin(), ks.end(), [&](size_t x, size_t y) {
        return ch.fo[fl[x].first].energy < ch.fo[fl[y].first].energy; });
      for (size_t k : ks) {
        vector<double> pop(8, 0.0);
        for (int q = fl[k].first; q <= fl[k].last; q++) {
          const size_t i = size_t(ch.fo[q].index - 1);
          if (i >= fmo->nmo) continue;
          const double* c = &fmo->coef[i * na];
          for (size_t mu = 0; mu < na; mu++) {
            double sc = 0;
            for (size_t nu = 0; nu < na; nu++) sc += Sf[mu * na + nu] * c[nu];
            if (lOf[mu] < 8) pop[lOf[mu]] += c[mu] * sc;
          }
        }
        const double n = double(fl[k].last - fl[k].first + 1);
        int best = 0;
        for (int l = 1; l < 8; l++) if (pop[l] > pop[best]) best = l;
        if (pop[best] / n >= 0.90) {
          const int nq = best + 1 + lBelow[best] / (2 * best + 1);
          flShell[k] = el + " " + std::to_string(nq) + "spdfghi"[best];
        }
        if (pop[best] / n >= 0.5) lBelow[best] += int(n);
      }
    }
    {
      std::map<string, int> count;
      for (size_t m = 0; m < nml && cok; m++) {
        string ir = csym.irrep.size() > size_t(ml[m].first) ? csym.irrep[ml[m].first] : "";
        for (int i = ml[m].first; i <= ml[m].last && !ir.empty(); i++)
          if (size_t(i) >= csym.irrep.size() || csym.irrep[i] != ir) ir.clear();
        if (!ir.empty()) mlIrrep[m] = std::to_string(++count[ir]) + lower(ir);
      }
      std::map<std::pair<int, string>, int> fcount;
      for (size_t k = 0; k < nfl; k++) {
        const int f = ch.fo[fl[k].first].fragment;
        if (!fok[f]) continue;
        const vector<string>& irs = fsym[f].irrep;
        string ir;
        for (int q = fl[k].first; q <= fl[k].last; q++) {
          const size_t i = size_t(ch.fo[q].index - 1);
          const string one = i < irs.size() ? irs[i] : "";
          if (q == fl[k].first) ir = one;
          else if (one != ir) ir.clear();
        }
        if (!ir.empty()) flIrrep[k] = std::to_string(++fcount[std::make_pair(f, ir)]) + lower(ir);
      }
    }
    // share of FO level k in MO level m
    vector<double> L(nfl * nml, 0.0);
    for (size_t k = 0; k < nfl; k++)
      for (size_t m = 0; m < nml; m++) {
        double s = 0;
        for (int q = fl[k].first; q <= fl[k].last; q++)
          for (int i = ml[m].first; i <= ml[m].last; i++) s += comp[size_t(q) * nmo + i];
        L[k * nml + m] = s / double(ml[m].last - ml[m].first + 1);
      }
    auto occupied = [&](size_t m) {
      for (int i = ml[m].first; i <= ml[m].last; i++) if (ch.occupation[i] > 1e-6) return true;
      return false;
    };
    int homoL = -1;
    for (size_t m = 0; m < nml; m++) if (occupied(m)) homoL = int(m);
    const int lumoL = homoL + 1 < int(nml) ? homoL + 1 : -1;

    vector<char> drawn(nml, 0), partner(nml, 0), drawnF(nfl, 0);
    for (int m = std::max(0, homoL - opt.occupiedLevels + 1); m <= homoL; m++) drawn[m] = 1;
    if (lumoL >= 0)
      for (int m = lumoL; m < std::min(int(nml), lumoL + opt.virtualLevels); m++) drawn[m] = 1;
    for (bool changed = true; changed;) {
      changed = false;
      for (size_t m = 0; m < nml; m++) {
        if (!drawn[m] || !occupied(m)) continue;
        for (size_t k = 0; k < nfl; k++) {
          if (L[k * nml + m] < opt.threshold) continue;
          for (size_t m2 = 0; m2 < nml; m2++)
            if (!drawn[m2] && L[k * nml + m2] >= opt.threshold) {
              drawn[m2] = partner[m2] = 1;
              changed = true;
            }
        }
      }
    }
    for (size_t k = 0; k < nfl; k++)
      for (size_t m = 0; m < nml && !drawnF[k]; m++)
        if (drawn[m] && L[k * nml + m] >= opt.threshold) drawnF[k] = 1;

    // HOMO (1-based index) of the complex and of each fragment
    int cxHomo = 0;
    for (size_t i = 0; i < nmo; i++) if (ch.occupation[i] > 1e-6) cxHomo = int(i + 1);
    vector<int> frHomo(nf, 0);
    for (size_t k = 0; k < nfo; k++) if (ch.fo[k].occupation > 1e-6) frHomo[ch.fo[k].fragment] = ch.fo[k].index;

    vector<int> moId(nml, -1), foId(nfl, -1);
    for (size_t m = 0; m < nml; m++) {
      if (!drawn[m]) continue;
      DiagramLevel lv;
      lv.id = int(dc.levels.size());
      lv.column = 1;
      lv.fragment = -1;
      lv.first = ml[m].first;
      lv.last = ml[m].last;
      double e = 0, rem = 0;
      for (int i = lv.first; i <= lv.last; i++) {
        lv.numbers.push_back(i + 1);
        lv.occupation.push_back(ch.occupation[i]);
        e += ch.energy[i];
        rem += ch.remainder[i];
      }
      const double n = double(lv.numbers.size());
      lv.energyEv = e / n * HARTREE_EV;
      lv.remainder = rem / n;
      lv.position = position(cxHomo, lv.numbers.front(), lv.numbers.back());
      lv.irrep = mlIrrep[m];
      lv.partner = partner[m] != 0;
      moId[m] = lv.id;
      dc.levels.push_back(lv);
    }
    for (size_t k = 0; k < nfl; k++) {
      if (!drawnF[k]) continue;
      DiagramLevel lv;
      lv.id = int(dc.levels.size());
      lv.fragment = ch.fo[fl[k].first].fragment;
      lv.column = lv.fragment == 0 ? 0 : 2;
      lv.first = fl[k].first;
      lv.last = fl[k].last;
      lv.kind = "fo";
      double e = 0;
      for (int q = lv.first; q <= lv.last; q++) {
        lv.numbers.push_back(ch.fo[q].index);
        lv.occupation.push_back(ch.fo[q].occupation);
        e += ch.fo[q].energy;
      }
      lv.energyEv = e / double(lv.numbers.size()) * HARTREE_EV;
      lv.position = position(frHomo[lv.fragment], lv.numbers.front(), lv.numbers.back());
      lv.irrep = flIrrep[k];
      lv.shell = flShell[k];
      foId[k] = lv.id;
      dc.levels.push_back(lv);
    }
    for (size_t k = 0; k < nfl; k++)
      for (size_t m = 0; m < nml; m++)
        if (foId[k] >= 0 && moId[m] >= 0 && L[k * nml + m] >= opt.threshold) {
          DiagramLine ln;
          ln.fo = foId[k];
          ln.mo = moId[m];
          ln.fragment = ch.fo[fl[k].first].fragment;
          ln.share = L[k * nml + m];
          dc.lines.push_back(ln);
        }

    // Per MO level: coverage, fragments, overlap population, kind, shares.
    for (size_t m = 0; m < nml; m++) {
      if (moId[m] < 0) continue;
      DiagramLevel& lv = dc.levels[moId[m]];
      std::set<int> frs;
      for (size_t t = 0; t < dc.lines.size(); t++)
        if (dc.lines[t].mo == lv.id) {
          lv.covered += dc.lines[t].share;
          frs.insert(dc.lines[t].fragment);
        }
      lv.fragments.assign(frs.begin(), frs.end());
      double opAll = 0, opConn = 0;
      for (int i = lv.first; i <= lv.last; i++)
        for (size_t f = 0; f < nf; f++)
          for (size_t g = f + 1; g < nf; g++) {
            const double op = dr.moOverlap[(moBase + size_t(i)) * np + size_t(pairIndex[f][g])];
            opAll += op;
            if (frs.count(int(f)) && frs.count(int(g))) opConn += op;
          }
      const double n = double(lv.last - lv.first + 1);
      lv.overlapPopulation = opAll / n;
      opConn /= n;
      if (frs.size() >= 2) lv.kind = opConn > 0 ? "bonding" : opConn < 0 ? "antibonding" : "zero overlap population";
      else if (frs.size() == 1) lv.kind = "fragment";
      else lv.kind = "unconnected";
      lv.representative = lv.covered >= opt.covered;
      if (!lv.representative) dc.notRepresentative.push_back(lv.id);
      vector<std::pair<double, size_t> > v;
      for (size_t k = 0; k < nfl; k++) if (L[k * nml + m] >= 0.01) v.push_back(std::make_pair(-L[k * nml + m], k));
      std::sort(v.begin(), v.end());
      for (size_t t = 0; t < v.size(); t++) {
        const size_t k = v[t].second;
        DiagramShare sh;
        sh.level = foId[k];
        sh.fragment = ch.fo[fl[k].first].fragment;
        double e = 0;
        for (int q = fl[k].first; q <= fl[k].last; q++) { sh.numbers.push_back(ch.fo[q].index); e += ch.fo[q].energy; }
        sh.energyEv = e / double(sh.numbers.size()) * HARTREE_EV;
        sh.share = -v[t].first;
        sh.position = position(frHomo[sh.fragment], sh.numbers.front(), sh.numbers.back());
        sh.irrep = flIrrep[k];
        sh.shell = flShell[k];
        lv.shares.push_back(sh);
      }
    }
    // Per FO level: the MO levels it goes into.
    for (size_t k = 0; k < nfl; k++) {
      if (foId[k] < 0) continue;
      DiagramLevel& lv = dc.levels[foId[k]];
      vector<std::pair<double, size_t> > v;
      for (size_t m = 0; m < nml; m++) if (L[k * nml + m] >= 0.01) v.push_back(std::make_pair(-L[k * nml + m], m));
      std::sort(v.begin(), v.end());
      for (size_t t = 0; t < v.size(); t++) {
        const size_t m = v[t].second;
        DiagramShare sh;
        sh.level = moId[m];
        double e = 0;
        for (int i = ml[m].first; i <= ml[m].last; i++) { sh.numbers.push_back(i + 1); e += ch.energy[i]; }
        sh.energyEv = e / double(sh.numbers.size()) * HARTREE_EV;
        sh.share = -v[t].first;
        sh.position = position(cxHomo, sh.numbers.front(), sh.numbers.back());
        sh.irrep = mlIrrep[m];
        lv.shares.push_back(sh);
      }
    }

    if (homoL >= 0) dc.homo = moId[homoL];
    if (lumoL >= 0) dc.lumo = moId[lumoL];
    if (dc.homo >= 0 && dc.lumo >= 0) dc.gapEv = dc.levels[dc.lumo].energyEv - dc.levels[dc.homo].energyEv;

    dc.representative = dc.notRepresentative.empty();
    // A far partner pulled in by the partner rule does not decide the
    // verdict on the window, but it is named with its number.
    const string pct = fmt("%.0f %%", 100 * opt.covered);
    string inWin, far;
    int nWin = 0, nFar = 0;
    for (size_t t = 0; t < dc.notRepresentative.size(); t++) {
      const DiagramLevel& lv = dc.levels[dc.notRepresentative[t]];
      string& to = lv.partner ? far : inWin;
      to += (to.empty() ? "" : ", ") + levelName("MO", lv.numbers) + fmt(" %.1f %%", 100 * lv.covered);
      (lv.partner ? nFar : nWin)++;
    }
    dc.windowRepresentative = nWin == 0;
    const char* words[] = {"no", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
    auto count = [&](int n, const char* one, const char* many) {
      return (n < 10 ? string(words[n]) : std::to_string(n)) + " " + (n == 1 ? one : many);
    };
    if (dc.representative)
      dc.verdict = "Representative: every drawn MO level is at least " + pct +
                   " made of the FO levels connected to it.";
    else if (nWin == 0)
      dc.verdict = "Representative in the window; " + count(nFar, "far partner level", "far partner levels") +
                   " (" + far + ") " + (nFar == 1 ? "is" : "are") + " less than " + pct +
                   " covered and drawn dashed.";
    else {
      dc.verdict = "Not representative in the window: " + count(nWin, "level", "levels") + " (" + inWin +
                   ") less than " + pct + " made of the FO levels connected to " + (nWin == 1 ? "it" : "them");
      if (nFar) dc.verdict += "; also " + count(nFar, "far partner level", "far partner levels") + " (" + far + ")";
      dc.verdict += "; drawn dashed.";
    }

    {
      string groups = "Point groups (Mulliken labels from the orbital coefficients): complex " +
                      (cok ? csym.group : "not determined (" + csym.why + ")");
      for (size_t f = 0; f < nf; f++)
      {
        const bool atom = f < a.fragmentModels.size() && a.fragmentModels[f] && a.fragmentModels[f]->natoms() == 1;
        groups += "; " + a.fragmentNames[f] + " " +
                  (fok[f] ? fsym[f].group
                   : atom ? "a single atom, its levels named by shell (one l at 90 % or more, else ?)"
                          : "not determined (" + fsym[f].why + ")");
      }
      if (opt.symmetry) dc.notes.push_back(groups + ".");
    }
    string partners;
    for (size_t i = 0; i < dc.levels.size(); i++)
      if (dc.levels[i].partner) partners += (partners.empty() ? "" : ", ") + levelName("MO", dc.levels[i].numbers);
    if (!partners.empty())
      dc.notes.push_back(partners + ": outside the window, drawn because they share an FO level with a drawn occupied MO level.");
    dc.hiddenFoLevels.assign(nf, 0);
    for (size_t k = 0; k < nfl; k++) if (!drawnF[k]) dc.hiddenFoLevels[ch.fo[fl[k].first].fragment]++;
    for (size_t f = 0; f < nf; f++) {
      if (!dc.hiddenFoLevels[f]) continue;
      string fr;
      for (size_t k = 0; k < nfl; k++) {
        const int g = ch.fo[fl[k].first].fragment;
        if (g != int(f) || drawnF[k]) continue;
        const int lo = ch.fo[fl[k].first].index, hi = ch.fo[fl[k].last].index;
        const string p = position(frHomo[f], lo, hi);
        if (p == "HOMO" || p == "LUMO") fr += string(fr.empty() ? "" : ", ") + "its " + p;
      }
      const bool one = dc.hiddenFoLevels[f] == 1;
      dc.notes.push_back(a.fragmentNames[f] + fmt(": %d FO level", dc.hiddenFoLevels[f]) + (one ? "" : "s") +
                         (fr.empty() ? "" : " (" + fr + ")") + (one ? " has" : " have") +
                         " no line to a drawn MO level and " + (one ? "is" : "are") + " not drawn.");
    }
    out.channel.push_back(dc);
    moBase += nmo;
  }
  return true;
}

namespace {

// Real solid harmonics with one norm per shell (Helgaker 6.4.47), in the
// results.h5 canonical order: p x,y,z; else m = 0, +1, -1, +2, -2, ...
// ShellRotation fits each operation's matrix from these polynomials, so they
// must be the AOs up to one factor per shell.
void addTerm(AngMomFunc& f, double c, int x, int y, int z) {
  Monomial m;
  m.m_coefficient = c; m.m_l = x; m.m_m = y; m.m_n = z; m.m_k = 0;
  f.push_back(m);
}

TGBSAngFunc* canonicalAngFunc() {
  TGBSAngFunc* a = new TGBSAngFunc;
  a->basisType(TGBSAngFunc::Spherical);
  a->componentNormalized(true);
  AngMomFunc f;
  addTerm(f, 1, 0, 0, 0); a->addFunc(f); a->markShell();
  for (int k = 0; k < 3; k++) { f.clear(); addTerm(f, 1, k == 0, k == 1, k == 2); a->addFunc(f); }
  a->markShell();
  const double r3 = std::sqrt(3.0);
  f.clear(); addTerm(f, 1, 0, 0, 2); addTerm(f, -0.5, 2, 0, 0); addTerm(f, -0.5, 0, 2, 0); a->addFunc(f);
  f.clear(); addTerm(f, r3, 1, 0, 1); a->addFunc(f);
  f.clear(); addTerm(f, r3, 0, 1, 1); a->addFunc(f);
  f.clear(); addTerm(f, r3 / 2, 2, 0, 0); addTerm(f, -r3 / 2, 0, 2, 0); a->addFunc(f);
  f.clear(); addTerm(f, r3, 1, 1, 0); a->addFunc(f);
  a->markShell();
  const double c1 = std::sqrt(3.0 / 8), c2 = std::sqrt(15.0) / 2, c3 = std::sqrt(5.0 / 8);
  f.clear(); addTerm(f, 1, 0, 0, 3); addTerm(f, -1.5, 2, 0, 1); addTerm(f, -1.5, 0, 2, 1); a->addFunc(f);
  f.clear(); addTerm(f, 4 * c1, 1, 0, 2); addTerm(f, -c1, 3, 0, 0); addTerm(f, -c1, 1, 2, 0); a->addFunc(f);
  f.clear(); addTerm(f, 4 * c1, 0, 1, 2); addTerm(f, -c1, 2, 1, 0); addTerm(f, -c1, 0, 3, 0); a->addFunc(f);
  f.clear(); addTerm(f, c2, 2, 0, 1); addTerm(f, -c2, 0, 2, 1); a->addFunc(f);
  f.clear(); addTerm(f, 2 * c2, 1, 1, 1); a->addFunc(f);
  f.clear(); addTerm(f, c3, 3, 0, 0); addTerm(f, -3 * c3, 1, 2, 0); a->addFunc(f);
  f.clear(); addTerm(f, 3 * c3, 2, 1, 0); addTerm(f, -c3, 0, 3, 0); a->addFunc(f);
  a->markShell();
  return a;
}

}  // namespace

bool orbitalSymmetry(const Model& m, int spin, OrbitalSymmetry& out) {
  out = OrbitalSymmetry();
  const Mos* mo = 0;
  for (size_t k = 0; k < m.mos.size(); k++) if (m.mos[k].spin == spin) mo = &m.mos[k];
  if (!mo && !m.mos.empty()) mo = &m.mos[0];
  if (!mo) { out.why = "no orbitals"; return false; }
  out.irrep.assign(mo->nmo, "");
  const size_t na = m.natoms();
  for (size_t k = 0; k < m.basis.shells.size(); k++) {
    if (m.basis.shells[k].cartesian) { out.why = "Cartesian basis functions are not labelled"; return false; }
    if (m.basis.shells[k].l > 3) { out.why = "g and higher shells are not labelled"; return false; }
  }
  const int g = mo->geometry;
  vector<double> stored(3 * na);
  vector<string> elements(na);
  std::ostringstream xyz;
  xyz << na << "\nfragorb\n";
  for (size_t a = 0; a < na; a++) {
    elements[a] = TPerTab().atomicSymbol(m.atomicNumber[a]);
    for (int x = 0; x < 3; x++) stored[3 * a + x] = m.coord[(size_t(g) * na + a) * 3 + x] * BOHR_ANG;
    xyz << elements[a] << " " << stored[3 * a] << " " << stored[3 * a + 1] << " " << stored[3 * a + 2] << "\n";
  }
  if (na == 1) { out.why = "a single atom: its spherical group is not in ECCE's character tables"; return false; }
  Fragment probe;
  std::istringstream in(xyz.str());
  if (!probe.restoreXYZ(in, 1.0, false)) { out.why = "the structure could not be read"; return false; }
  try {
    out.group = SymmetryOps::find(probe, 0.01);
  } catch (...) {
    out.why = "the symmetry search (autosym) could not be run";
    return false;
  }
  const CharacterTable* table = CharacterTable::lookup(out.group);
  if (!table) { out.why = "no character table for " + out.group; return false; }
  vector<double> probeCoords(3 * na);
  const double* pc = probe.coordinates();
  if (!pc || probe.numAtoms() != na) { out.why = "the symmetry search changed the atoms"; return false; }
  for (size_t i = 0; i < 3 * na; i++) probeCoords[i] = pc[i];
  vector<SymOp> ops;
  if (!MoFragments::symmetryOperations(out.group, ops)) { out.why = "no operations for " + out.group + " (symops)"; return false; }
  vector<vector<int> > images, classes;
  if (!SymmetryAnalysis::atomImages(probeCoords, elements, ops, 0.05, images)) { out.why = "the operations do not map the atoms"; return false; }
  SymmetryAnalysis::conjugacyClasses(ops, classes);
  vector<int> classOfOp;
  if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp)) { out.why = "the classes of " + out.group + " do not match"; return false; }
  SymOp Q;
  double rmsd = 0;
  if (na < 3) { out.why = "fewer than three atoms: the frame cannot be fixed"; return false; }
  if (!SymmetryAnalysis::alignFrames(stored, probeCoords, Q, rmsd) || rmsd > 0.1) {
    out.why = "the symmetrised frame does not match the calculation's";
    return false;
  }
  vector<SymOp> opsInFrame(ops.size());
  for (size_t i = 0; i < ops.size(); i++) opsInFrame[i] = SymmetryAnalysis::conjugate(ops[i], Q);

  // AO bookkeeping in the canonical order
  vector<int> perAtom(na, 0), shellType;
  for (size_t k = 0; k < m.basis.shells.size(); k++) {
    const Shell& sh = m.basis.shells[k];
    const int n = 2 * sh.l + 1;
    perAtom[sh.atom] += n;
    for (int i = 0; i < n; i++) shellType.push_back(sh.l);
  }
  const size_t nao = shellType.size();
  if (nao != mo->nao) { out.why = "basis and orbital sizes differ"; return false; }
  vector<double> S;
  string err;
  if (!overlapMatrix(m, g, S, err)) { out.why = "overlap: " + err; return false; }
  vector<vector<double> > S2(nao, vector<double>(nao));
  for (size_t i = 0; i < nao; i++) for (size_t j = 0; j < nao; j++) S2[i][j] = S[i * nao + j];
  vector<vector<double> > orbitals(mo->nmo, vector<double>(nao));
  for (size_t i = 0; i < mo->nmo; i++) for (size_t p = 0; p < nao; p++) orbitals[i][p] = mo->coef[i * nao + p];
  TGBSAngFunc* ang = canonicalAngFunc();
  vector<string> derived;
  SymmetryAnalysis::fullLabelSpectrum(orbitals, mo->energy, perAtom, shellType, S2, images, classOfOp,
                                      opsInFrame, ang, *table, 1e-4, derived);
  delete ang;
  for (size_t i = 0; i < derived.size() && i < out.irrep.size(); i++) out.irrep[i] = derived[i];
  return true;
}

}
