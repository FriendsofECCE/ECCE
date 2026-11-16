// Natural population analysis from results.h5 (#1054); see the header.
// The overlap is resultsh5::overlapMatrix (EspField::overlapOf), so the
// AO metric is the one every other results.h5 view uses.
#include "tdat/NaturalPopulation.H"
#include "tdat/MoComposition.H"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>

namespace npa {

using resultsh5::Model;

static const char* SYMBOL[] = { "X",
  "H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar",
  "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
  "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe",
  "Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu",
  "Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn" };
static const int MAXZ = 86;
static const char LCHAR[] = "spdfghi";

typedef std::pair<int,int> NL;   // (n, l)

// Subshells the free atom occupies in its ground configuration (Madelung).
static vector<NL> occupiedShells(int z) {
  static const NL order[] = {{1,0},{2,0},{2,1},{3,0},{3,1},{4,0},{3,2},{4,1},{5,0},
    {4,2},{5,1},{6,0},{4,3},{5,2},{6,1},{7,0},{5,3},{6,2},{7,1}};
  vector<NL> out;
  int left = z;
  for (const NL& s : order) {
    if (left <= 0) break;
    out.push_back(s);
    left -= 2*(2*s.second + 1);
  }
  return out;
}

// Core subshells: the preceding noble gas, plus a filled (n-1)d after
// group 12 and 4f from Hf on (the NBO convention).
static vector<NL> coreShells(int z) {
  vector<NL> core;
  const int gas[] = {2, 10, 18, 36, 54, 86};
  int ngas = 0;
  for (int g : gas) if (z > g) ngas = g;
  for (const NL& s : occupiedShells(ngas)) core.push_back(s);
  if (z >= 31 && z <= 36) core.push_back(NL(3,2));
  if (z >= 49 && z <= 54) core.push_back(NL(4,2));
  if (z >= 72) core.push_back(NL(4,3));
  if (z >= 81) core.push_back(NL(5,2));
  std::sort(core.begin(), core.end());
  core.erase(std::unique(core.begin(), core.end()), core.end());
  return core;
}

string notApplicable(const Model& m) {
  if (m.basis.orthonormal)
    return "semiempirical (orthonormal basis): natural populations equal the Loewdin populations";
  if (m.hasCell && (m.pbc[0] || m.pbc[1] || m.pbc[2]))
    return "periodic calculation: NPA needs a molecular basis";
  if (m.basis.type != "GTO" || m.basis.shells.empty())
    return "no Gaussian basis set in results.h5";
  if (m.mos.empty()) return "no orbitals in results.h5";
  for (const resultsh5::Shell& s : m.basis.shells)
    if (s.cartesian && s.l >= 2)
      return "Cartesian d/f shells are not supported yet";
  return "";
}

// ---- small dense linear algebra (row-major, flat) -------------------------
typedef vector<double> Mat;

static void symEig(const Mat& a, int n, vector<double>& val, Mat& vec) {
  vector<vector<double> > A(n, vector<double>(n)), V;
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) A[i][j] = 0.5*(a[i*n+j] + a[j*n+i]);
  MoComposition::jacobiEigen(A, val, V);
  vec.assign(size_t(n)*n, 0.0);
  for (int i = 0; i < n; i++) for (int k = 0; k < n; k++) vec[i*n+k] = V[i][k];
}

// C^T M C, C is nr x nc, M nr x nr
static Mat congruence(const Mat& C, int nr, int nc, const Mat& M) {
  Mat MC(size_t(nr)*nc, 0.0), out(size_t(nc)*nc, 0.0);
  for (int i = 0; i < nr; i++)
    for (int k = 0; k < nr; k++) {
      const double mik = M[i*nr+k];
      if (mik == 0) continue;
      for (int j = 0; j < nc; j++) MC[i*nc+j] += mik*C[k*nc+j];
    }
  for (int k = 0; k < nr; k++)
    for (int i = 0; i < nc; i++) {
      const double cki = C[k*nc+i];
      if (cki == 0) continue;
      for (int j = 0; j < nc; j++) out[i*nc+j] += cki*MC[k*nc+j];
    }
  return out;
}

// (M)^(-1/2) of a symmetric positive matrix
static bool invSqrt(const Mat& M, int n, Mat& out) {
  vector<double> val; Mat vec;
  symEig(M, n, val, vec);
  out.assign(size_t(n)*n, 0.0);
  for (int k = 0; k < n; k++) {
    if (val[k] < 1e-14) return false;
    const double f = 1.0/std::sqrt(val[k]);
    for (int i = 0; i < n; i++) {
      const double vi = vec[i*n+k]*f;
      for (int j = 0; j < n; j++) out[i*n+j] += vi*vec[j*n+k];
    }
  }
  return true;
}

// ---- the function set: columns of C over the AOs ---------------------------
struct Fn { int atom, l, m, radial; };

struct Set {
  int nao = 0;
  vector<Fn> fn;            // one per column
  Mat C;                    // nao x fn.size(), AO coefficients (column j = fn j)
  int ncol() const { return int(fn.size()); }
  vector<double> col(int j) const { vector<double> c(nao); for (int i = 0; i < nao; i++) c[i] = C[size_t(i)*ncol()+j]; return c; }
};

static Set subset(const Set& s, const vector<int>& cols) {
  Set o; o.nao = s.nao;
  for (int j : cols) o.fn.push_back(s.fn[j]);
  o.C.assign(size_t(s.nao)*cols.size(), 0.0);
  for (int i = 0; i < s.nao; i++)
    for (size_t k = 0; k < cols.size(); k++) o.C[i*cols.size()+k] = s.C[size_t(i)*s.ncol()+cols[k]];
  return o;
}

static Set join(const Set& a, const Set& b) {
  Set o; o.nao = a.nao;
  o.fn = a.fn; o.fn.insert(o.fn.end(), b.fn.begin(), b.fn.end());
  const int na = a.ncol(), nb = b.ncol(), n = na + nb;
  o.C.assign(size_t(a.nao)*n, 0.0);
  for (int i = 0; i < a.nao; i++) {
    for (int j = 0; j < na; j++) o.C[i*n+j] = a.C[size_t(i)*na+j];
    for (int j = 0; j < nb; j++) o.C[i*n+na+j] = b.C[size_t(i)*nb+j];
  }
  return o;
}

// Replace the columns by C T (T is ncol x ncol).
static void transform(Set& s, const Mat& T) {
  const int n = s.ncol();
  Mat out(size_t(s.nao)*n, 0.0);
  for (int i = 0; i < s.nao; i++)
    for (int k = 0; k < n; k++) {
      const double c = s.C[size_t(i)*n+k];
      if (c == 0) continue;
      for (int j = 0; j < n; j++) out[size_t(i)*n+j] += c*T[k*n+j];
    }
  s.C.swap(out);
}

// Natural (m-averaged) diagonalisation of each atom's l block in the S
// metric.  Radial functions are renumbered by decreasing occupancy; occ[j]
// is the occupancy of column j's orbital.
static void naturalBlocks(Set& s, const Mat& S, const Mat& D, vector<double>& occ) {
  const int n = s.ncol();
  Mat Sf = congruence(s.C, s.nao, n, S), Df = congruence(s.C, s.nao, n, D);
  std::map<std::pair<int,int>, vector<vector<int> > > groups;  // (atom,l) -> [radial][m] -> col
  for (int j = 0; j < n; j++) {
    vector<vector<int> >& g = groups[std::make_pair(s.fn[j].atom, s.fn[j].l)];
    if (int(g.size()) <= s.fn[j].radial) g.resize(s.fn[j].radial + 1, vector<int>(2*s.fn[j].l + 1, -1));
    g[s.fn[j].radial][s.fn[j].m] = j;
  }
  Mat T(size_t(n)*n, 0.0);
  occ.assign(n, 0.0);
  for (auto& kv : groups) {
    const int l = kv.first.second, nm = 2*l + 1;
    const vector<vector<int> >& g = kv.second;
    const int k = int(g.size());
    Mat Sb(size_t(k)*k, 0.0), Db(size_t(k)*k, 0.0);
    for (int a = 0; a < k; a++)
      for (int b = 0; b < k; b++)
        for (int m = 0; m < nm; m++) {
          Sb[a*k+b] += Sf[g[a][m]*n + g[b][m]]/nm;
          Db[a*k+b] += Df[g[a][m]*n + g[b][m]]/nm;
        }
    // generalised problem through S^-1/2 (canonical, near-null directions dropped
    // only as far as keeping k vectors allows)
    Mat X;
    if (!invSqrt(Sb, k, X)) {
      vector<double> sv; Mat svec; symEig(Sb, k, sv, svec);
      X.assign(size_t(k)*k, 0.0);
      for (int q = 0; q < k; q++) {
        const double f = 1.0/std::sqrt(std::max(sv[q], 1e-14));
        for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) X[i*k+j] += svec[i*k+q]*f*svec[j*k+q];
      }
    }
    Mat H = congruence(X, k, k, Db);
    vector<double> val; Mat vec;
    symEig(H, k, val, vec);
    vector<int> order(k);
    for (int q = 0; q < k; q++) order[q] = q;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return val[a] > val[b]; });
    for (int r = 0; r < k; r++) {
      const int q = order[r];
      vector<double> v(k, 0.0);   // X u
      for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) v[i] += X[i*k+j]*vec[j*k+q];
      for (int m = 0; m < nm; m++) {
        const int dst = g[r][m];
        for (int a = 0; a < k; a++) T[g[a][m]*n + dst] = v[a];
        occ[dst] = val[q];
      }
    }
  }
  transform(s, T);
}

// Occupancy-weighted symmetric orthogonalisation: C W (W S W)^-1/2.
static bool owso(Set& s, const Mat& S, const vector<double>& w) {
  const int n = s.ncol();
  if (n == 0) return true;
  Mat Sf = congruence(s.C, s.nao, n, S);
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) Sf[i*n+j] *= w[i]*w[j];
  Mat X;
  if (!invSqrt(Sf, n, X)) return false;
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) X[i*n+j] *= w[i];
  transform(s, X);
  return true;
}

// Project the minimal set out of the Rydberg set: R - N (N'SN)^-1 N'SR.
static bool schmidt(Set& R, const Set& N, const Mat& S) {
  const int nn = N.ncol(), nr = R.ncol(), nao = R.nao;
  if (nn == 0 || nr == 0) return true;
  Mat SN(size_t(nao)*nn, 0.0);   // S N
  for (int i = 0; i < nao; i++)
    for (int k = 0; k < nao; k++) {
      const double sik = S[size_t(i)*nao+k];
      if (sik == 0) continue;
      for (int j = 0; j < nn; j++) SN[size_t(i)*nn+j] += sik*N.C[size_t(k)*nn+j];
    }
  Mat NN(size_t(nn)*nn, 0.0), NR(size_t(nn)*nr, 0.0);
  for (int k = 0; k < nao; k++)
    for (int i = 0; i < nn; i++) {
      const double v = SN[size_t(k)*nn+i];
      for (int j = 0; j < nn; j++) NN[i*nn+j] += v*N.C[size_t(k)*nn+j];
      for (int j = 0; j < nr; j++) NR[size_t(i)*nr+j] += v*R.C[size_t(k)*nr+j];
    }
  Mat Xh;
  if (!invSqrt(NN, nn, Xh)) return false;
  Mat inv(size_t(nn)*nn, 0.0);
  for (int i = 0; i < nn; i++) for (int k = 0; k < nn; k++) for (int j = 0; j < nn; j++)
    inv[i*nn+j] += Xh[i*nn+k]*Xh[k*nn+j];
  Mat A(size_t(nn)*nr, 0.0);
  for (int i = 0; i < nn; i++) for (int k = 0; k < nn; k++) {
    const double v = inv[i*nn+k];
    for (int j = 0; j < nr; j++) A[size_t(i)*nr+j] += v*NR[size_t(k)*nr+j];
  }
  for (int a = 0; a < nao; a++)
    for (int i = 0; i < nn; i++) {
      const double v = N.C[size_t(a)*nn+i];
      if (v == 0) continue;
      for (int j = 0; j < nr; j++) R.C[size_t(a)*nr+j] -= v*A[size_t(i)*nr+j];
    }
  return true;
}

static Mat densityAo(const resultsh5::Mos& mo) {
  const size_t n = mo.nao;
  Mat P(n*n, 0.0);
  for (size_t k = 0; k < mo.nmo && k < mo.occupation.size(); k++) {
    const double o = mo.occupation[k];
    if (std::fabs(o) < 1e-12) continue;
    const double* c = &mo.coef[k*n];
    for (size_t i = 0; i < n; i++) {
      const double oc = o*c[i];
      for (size_t j = 0; j < n; j++) P[i*n+j] += oc*c[j];
    }
  }
  return P;
}

// Ryd weights below this make (WSW) singular to rounding; such orbitals hold
// no electrons, so the floor changes no population.
static const double MIN_WEIGHT = 1e-6;

bool compute(const Model& m, Result& r, string& err) {
  r = Result();
  string why = notApplicable(m);
  if (!why.empty()) { err = why; return false; }
  const int natoms = int(m.natoms());

  // orbitals of the last geometry that has alpha orbitals
  int geom = -1;
  for (const resultsh5::Mos& mo : m.mos) if (mo.spin == 0) geom = std::max(geom, mo.geometry);
  const resultsh5::Mos *ma = 0, *mb = 0;
  for (const resultsh5::Mos& mo : m.mos) {
    if (mo.geometry != geom) continue;
    if (mo.spin == 0 && !ma) ma = &mo;
    if (mo.spin == 1 && !mb) mb = &mo;
  }
  if (!ma) { err = "no alpha orbitals in results.h5"; return false; }
  const int nao = int(ma->nao);
  if (int(m.basis.nao()) != nao) { err = "basis and orbitals differ in size"; return false; }
  Mat S;
  if (!resultsh5::overlapMatrix(m, geom, S, err)) return false;

  Mat Pa = densityAo(*ma), Pb;
  if (mb) Pb = densityAo(*mb);
  else { for (double& x : Pa) x *= 0.5; Pb = Pa; }
  r.openShell = mb != 0;
  Mat SPS[2];
  for (int sp = 0; sp < 2; sp++) {
    const Mat& P = sp ? Pb : Pa;
    Mat SP(size_t(nao)*nao, 0.0);
    for (int i = 0; i < nao; i++) for (int k = 0; k < nao; k++) {
      const double s = S[size_t(i)*nao+k];
      if (s == 0) continue;
      for (int j = 0; j < nao; j++) SP[size_t(i)*nao+j] += s*P[size_t(k)*nao+j];
    }
    SPS[sp].assign(size_t(nao)*nao, 0.0);
    for (int i = 0; i < nao; i++) for (int k = 0; k < nao; k++) {
      const double v = SP[size_t(i)*nao+k];
      if (v == 0) continue;
      for (int j = 0; j < nao; j++) SPS[sp][size_t(i)*nao+j] += v*S[size_t(k)*nao+j];
    }
  }
  Mat D(size_t(nao)*nao);
  for (size_t i = 0; i < D.size(); i++) D[i] = SPS[0][i] + SPS[1][i];
  double nel = 0;
  for (int sp = 0; sp < 2; sp++) {
    const resultsh5::Mos* mo = sp ? mb : ma;
    if (!mo) continue;
    for (double o : mo->occupation) nel += o;
  }

  // ECP core per atom: the file's ecp group, its effective charges, or
  // (files written from the parsed properties carry neither) the def2
  // Stuttgart cores when the all-electron count cannot be right.
  vector<int> ncore(natoms, 0);
  if (!m.ecp.atom.empty()) {
    for (size_t k = 0; k < m.ecp.atom.size() && k < m.ecp.ncore.size(); k++)
      if (m.ecp.atom[k] >= 0 && m.ecp.atom[k] < natoms) ncore[m.ecp.atom[k]] = m.ecp.ncore[k];
  } else if (int(m.nuclearCharge.size()) == natoms) {
    for (int a = 0; a < natoms; a++) ncore[a] = int(std::lround(m.atomicNumber[a] - m.nuclearCharge[a]));
  } else {
    double zsum = 0;
    for (int a = 0; a < natoms; a++) zsum += m.atomicNumber[a];
    if (std::fabs(zsum - nel) > 8) {
      vector<int> guess(natoms, 0);
      double zeff = 0;
      for (int a = 0; a < natoms; a++) {
        const int z = m.atomicNumber[a];
        guess[a] = (z >= 37 && z <= 54) ? 28 : (z >= 55 && z <= 57) ? 46 : (z >= 72 && z <= 86) ? 60 : 0;
        zeff += z - guess[a];
      }
      if (std::fabs(zeff - nel) <= 8 && std::fabs(zeff - nel - std::lround(zeff - nel)) < 1e-3) {
        ncore = guess;
        r.note = "ECP cores not in results.h5; def2 (Stuttgart) core sizes assumed";
      }
    }
  }

  // AO functions
  Set ao; ao.nao = nao;
  {
    std::map<std::pair<int,int>, int> radial;
    for (const resultsh5::Shell& sh : m.basis.shells) {
      const int nm = 2*sh.l + 1;
      const int rad = radial[std::make_pair(sh.atom, sh.l)]++;
      for (int q = 0; q < nm; q++) ao.fn.push_back(Fn{sh.atom, sh.l, q, rad});
    }
    if (ao.ncol() != nao) { err = "basis shells do not match the AO count"; return false; }
    ao.C.assign(size_t(nao)*nao, 0.0);
    for (int i = 0; i < nao; i++) ao.C[size_t(i)*nao+i] = 1.0;
  }

  // per atom: minimal-basis (n,l) list after the ECP core
  vector<vector<NL> > nmbShells(natoms), coreList(natoms);
  for (int a = 0; a < natoms; a++) {
    const int z = m.atomicNumber[a];
    if (z < 1 || z > MAXZ) { err = "element Z=" + std::to_string(z) + " not supported"; return false; }
    vector<NL> occd = occupiedShells(z), core = coreShells(z);
    int left = ncore[a];
    vector<NL> gone;
    for (const NL& s : core) {
      const int cap = 2*(2*s.second + 1);
      if (left >= cap) { gone.push_back(s); left -= cap; }
    }
    if (left != 0) { err = string(SYMBOL[z]) + ": ECP core of " + std::to_string(ncore[a]) + " electrons is not a set of closed shells"; return false; }
    for (const NL& s : occd) if (std::find(gone.begin(), gone.end(), s) == gone.end()) nmbShells[a].push_back(s);
    for (const NL& s : core) if (std::find(gone.begin(), gone.end(), s) == gone.end()) coreList[a].push_back(s);
  }

  // 1. pre-NAOs
  vector<double> occ;
  naturalBlocks(ao, S, D, occ);

  // 2. minimal basis = the highest-occupancy pre-NAOs per (atom,l)
  vector<int> colN, colR;
  for (int j = 0; j < ao.ncol(); j++) {
    const Fn& f = ao.fn[j];
    int nmb = 0;
    for (const NL& s : nmbShells[f.atom]) if (s.second == f.l) nmb++;
    (f.radial < nmb ? colN : colR).push_back(j);
  }
  Set N = subset(ao, colN), R = subset(ao, colR);
  // OWSO weights: the pre-NAO occupancies (m-averaged, as the orbitals are)
  vector<double> wN;
  for (int j : colN) wN.push_back(std::max(occ[j], MIN_WEIGHT));
  auto countL = [](const vector<NL>& v, int l) { int c = 0; for (const NL& s : v) if (s.second == l) c++; return c; };
  for (Fn& f : R.fn) f.radial -= countL(nmbShells[f.atom], f.l);   // Rydberg radials from 0

  // 3. Minimal basis: the core set is orthogonalised on its own and the
  //    valence set Schmidt-orthogonalised to it before its weighted
  //    orthogonalisation, so core orbitals stay atomic (NBO does the same:
  //    with the core inside one OWSO, O in water differs from NBO's NPA by
  //    0.002 e and its core population by 1e-4).
  {
    vector<int> cc, vv;
    vector<double> wc, wv;
    for (int j = 0; j < N.ncol(); j++) {
      const bool core = N.fn[j].radial < countL(coreList[N.fn[j].atom], N.fn[j].l);
      (core ? cc : vv).push_back(j);
      (core ? wc : wv).push_back(wN[j]);
    }
    Set Cs = subset(N, cc), Vs = subset(N, vv);
    if (!owso(Cs, S, wc) || !schmidt(Vs, Cs, S) || !owso(Vs, S, wv)) {
      err = "weighted orthogonalisation of the minimal basis failed"; return false;
    }
    N = join(Cs, Vs);
  }
  //    Rydberg set: Schmidt against the minimal set, natural character
  //    restored (its occupancies are the weights), weighted orthogonalisation.
  if (!schmidt(R, N, S)) { err = "minimal basis is linearly dependent"; return false; }
  vector<double> occR;
  naturalBlocks(R, S, D, occR);
  for (double& x : occR) x = std::max(x, MIN_WEIGHT);
  if (!owso(R, S, occR)) { err = "weighted orthogonalisation of the Rydberg set failed (linear dependence in the basis?)"; return false; }

  // 4. NAOs: re-diagonalised atomic blocks of the orthonormal set
  for (Fn& f : R.fn) f.radial += countL(nmbShells[f.atom], f.l);
  Set all = join(N, R);
  naturalBlocks(all, S, D, occ);
  const int nf = all.ncol();
  Mat Dsp[2] = { congruence(all.C, nao, nf, SPS[0]), congruence(all.C, nao, nf, SPS[1]) };

  // 5. populations, labels, configuration
  r.atoms.resize(natoms);
  std::map<std::pair<int,std::pair<int,int> >, int> shellIndex;   // (atom,(l,radial)) -> shells[]
  for (int a = 0; a < natoms; a++) {
    AtomResult& at = r.atoms[a];
    at.z = m.atomicNumber[a];
    at.symbol = SYMBOL[at.z];
    at.ecpCore = ncore[a];
    at.zeff = at.z - ncore[a];
  }
  for (int j = 0; j < nf; j++) {
    const Fn& f = all.fn[j];
    AtomResult& at = r.atoms[f.atom];
    auto key = std::make_pair(f.atom, std::make_pair(f.l, f.radial));
    auto it = shellIndex.find(key);
    if (it == shellIndex.end()) {
      NaoShell sh; sh.l = f.l;
      vector<int> nmbN;
      for (const NL& s : nmbShells[f.atom]) if (s.second == f.l) nmbN.push_back(s.first);
      if (f.radial < int(nmbN.size())) {
        sh.n = nmbN[f.radial];
        const vector<NL>& cl = coreList[f.atom];
        sh.kind = std::find(cl.begin(), cl.end(), NL(sh.n, f.l)) != cl.end() ? "Cor" : "Val";
      } else {
        int nval = 0;
        for (const NL& s : nmbShells[f.atom]) nval = std::max(nval, s.first);
        int start = nmbN.empty() ? std::max(nval + 1, f.l + 1) : nmbN.back() + 1;
        sh.n = start + f.radial - int(nmbN.size());
        sh.kind = "Ryd";
      }
      it = shellIndex.insert(std::make_pair(key, int(at.shells.size()))).first;
      at.shells.push_back(sh);
    }
    NaoShell& sh = at.shells[it->second];
    const double pa = Dsp[0][size_t(j)*nf+j], pb = Dsp[1][size_t(j)*nf+j];
    sh.alpha += pa; sh.beta += pb; sh.occ += pa + pb;
  }
  for (AtomResult& at : r.atoms) {
    std::sort(at.shells.begin(), at.shells.end(), [](const NaoShell& x, const NaoShell& y) {
      return x.n != y.n ? x.n < y.n : x.l < y.l; });
    std::ostringstream cfg;
    bool anyCore = false;
    for (const NaoShell& sh : at.shells) {
      at.alpha += sh.alpha; at.beta += sh.beta;
      if (sh.kind == "Cor") { at.core += sh.occ; anyCore = true; }
      else if (sh.kind == "Val") at.valence += sh.occ;
      else at.rydberg += sh.occ;
    }
    at.population = at.alpha + at.beta;
    at.charge = at.zeff - at.population;
    r.electrons += at.population;
    if (anyCore) cfg << "[core]";
    char buf[64];
    for (const NaoShell& sh : at.shells) {
      if (sh.kind == "Cor") continue;
      if (sh.kind == "Ryd" && sh.occ < 0.005) continue;
      snprintf(buf, sizeof buf, "%d%c(%.2f)", sh.n, LCHAR[sh.l], sh.occ);
      cfg << buf;
    }
    at.configuration = cfg.str();
  }
  if (std::fabs(r.electrons - nel) > 1e-4) {
    char buf[128];
    snprintf(buf, sizeof buf, "natural populations sum to %.6f, the orbitals hold %.6f electrons", r.electrons, nel);
    err = buf;
    return false;
  }
  return true;
}

static string jsonEsc(const string& s) {
  string o;
  for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
  return o;
}

string toJson(const Result& r) {
  std::ostringstream o;
  char b[256];
  snprintf(b, sizeof b, "{\n  \"method\": \"NPA (Reed, Weinstock, Weinhold 1985)\",\n  \"electrons\": %.6f,\n  \"open_shell\": %s,\n",
           r.electrons, r.openShell ? "true" : "false");
  o << b;
  if (!r.note.empty()) o << "  \"note\": \"" << jsonEsc(r.note) << "\",\n";
  o << "  \"atoms\": [\n";
  for (size_t a = 0; a < r.atoms.size(); a++) {
    const AtomResult& at = r.atoms[a];
    snprintf(b, sizeof b, "    {\"index\": %zu, \"element\": \"%s\", \"charge\": %.6f, \"population\": %.6f, "
             "\"core\": %.6f, \"valence\": %.6f, \"rydberg\": %.6f, \"spin\": %.6f, \"ecp_core\": %d, ",
             a + 1, at.symbol.c_str(), at.charge, at.population, at.core, at.valence, at.rydberg,
             at.alpha - at.beta, at.ecpCore);
    o << b << "\"configuration\": \"" << jsonEsc(at.configuration) << "\", \"shells\": [";
    for (size_t k = 0; k < at.shells.size(); k++) {
      const NaoShell& sh = at.shells[k];
      snprintf(b, sizeof b, "%s{\"shell\": \"%d%c\", \"type\": \"%s\", \"occupancy\": %.6f, \"spin\": %.6f}",
               k ? ", " : "", sh.n, LCHAR[sh.l], sh.kind.c_str(), sh.occ, sh.alpha - sh.beta);
      o << b;
    }
    o << "]}" << (a + 1 < r.atoms.size() ? "," : "") << "\n";
  }
  o << "  ]\n}\n";
  return o.str();
}

string toText(const Result& r) {
  std::ostringstream o;
  char b[256];
  o << "Natural population analysis (NPA), computed by ECCE\n";
  if (!r.note.empty()) o << "Note: " << r.note << "\n";
  snprintf(b, sizeof b, "%5s %-3s %10s %10s %10s %10s %10s%s\n", "Atom", "", "Charge", "Core",
           "Valence", "Rydberg", "Total", r.openShell ? "       Spin" : "");
  o << b;
  double qsum = 0;
  for (size_t a = 0; a < r.atoms.size(); a++) {
    const AtomResult& at = r.atoms[a];
    qsum += at.charge;
    snprintf(b, sizeof b, "%5zu %-3s %10.5f %10.5f %10.5f %10.5f %10.5f", a + 1, at.symbol.c_str(),
             at.charge, at.core, at.valence, at.rydberg, at.population);
    o << b;
    if (r.openShell) { snprintf(b, sizeof b, " %10.5f", at.alpha - at.beta); o << b; }
    o << "\n";
  }
  snprintf(b, sizeof b, "Total charge %.5f, electrons %.5f\n\nNatural electron configuration\n", qsum, r.electrons);
  o << b;
  for (size_t a = 0; a < r.atoms.size(); a++) {
    snprintf(b, sizeof b, "%5zu %-3s ", a + 1, r.atoms[a].symbol.c_str());
    o << b << r.atoms[a].configuration << "\n";
  }
  return o.str();
}

}
