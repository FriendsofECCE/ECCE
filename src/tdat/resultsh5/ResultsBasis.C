// Canonical AOs, overlap, invariants.  The overlap integrals are
// EspField::overlapOf -- the one implementation the viewer also uses.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "tdat/EspField.H"
#include "Internal.H"

namespace resultsh5 {

size_t Basis::nao() const {
  size_t n = 0;
  for (size_t i = 0; i < shells.size(); i++) n += shellSize(shells[i]);
  return n;
}

int shellSize(const Shell& s) {
  return s.cartesian ? (s.l+1)*(s.l+2)/2 : 2*s.l+1;
}

vector<Pow> cartCanonical(int l) {
  vector<Pow> v;
  for (int a = l; a >= 0; a--)
    for (int b = l-a; b >= 0; b--) v.push_back(Pow{{a, b, l-a-b}});
  return v;
}

vector<Pow> cartMolden(int l) {
  static const int t2[6][3] = {{2,0,0},{0,2,0},{0,0,2},{1,1,0},{1,0,1},{0,1,1}};
  static const int t3[10][3] = {{3,0,0},{0,3,0},{0,0,3},{1,2,0},{2,1,0},{2,0,1},
                                {1,0,2},{0,1,2},{0,2,1},{1,1,1}};
  static const int t4[15][3] = {{4,0,0},{0,4,0},{0,0,4},{3,1,0},{3,0,1},{1,3,0},
                                {0,3,1},{1,0,3},{0,1,3},{2,2,0},{2,0,2},{0,2,2},
                                {2,1,1},{1,2,1},{1,1,2}};
  vector<Pow> v;
  if (l <= 1) return cartCanonical(l);
  int n = (l+1)*(l+2)/2;
  const int (*t)[3] = l == 2 ? t2 : l == 3 ? t3 : t4;
  for (int i = 0; i < n; i++) v.push_back(Pow{{t[i][0], t[i][1], t[i][2]}});
  return v;
}

vector<int> cartPermFromMolden(int l) {
  vector<Pow> c = cartCanonical(l), s = cartMolden(l);
  vector<int> p;
  for (size_t i = 0; i < s.size(); i++)
    p.push_back(int(std::find(c.begin(), c.end(), s[i]) - c.begin()));
  return p;
}

double nrad(double a, int l) {
  double df = 1;
  for (int k = 2*l-1; k > 1; k -= 2) df *= k;
  return std::pow(2*a/M_PI, 0.75)*std::pow(4*a, 0.5*l)/std::sqrt(df);
}

namespace {

struct Term { int a, b, c; double w; };

double binom(int n, int k) {
  if (k < 0 || k > n) return 0;
  double r = 1;
  for (int i = 1; i <= k; i++) r = r*(n-k+i)/i;
  return r;
}

EspBasisFunction fromTerms(const vector<Term>& t, const double* c) {
  EspBasisFunction f;
  for (int i = 0; i < 3; i++) f.center[i] = c[i];
  for (size_t i = 0; i < t.size(); i++) {
    f.powerX.push_back(t[i].a); f.powerY.push_back(t[i].b);
    f.powerZ.push_back(t[i].c); f.angularCoef.push_back(t[i].w);
  }
  return f;
}

// Real solid harmonics (Helgaker 6.4.47), component order m = 0,+1,-1,+2,-2...
// Scaled so one primitive has the self-overlap of x^l: the convention the
// Gaussian-family programs use for their pure functions.
const vector<vector<Term> >& sphericalTerms(int l) {
  static vector<vector<vector<Term> > > cache(8);
  vector<vector<Term> >& out = cache[l];
  if (!out.empty()) return out;
  double c0[3] = {0,0,0};
  EspBasisFunction xl;
  xl.powerX.push_back(l); xl.powerY.push_back(0); xl.powerZ.push_back(0);
  xl.angularCoef.push_back(1); xl.exponent.push_back(1); xl.contraction.push_back(1);
  const double target = EspField::overlapOf(xl, xl);
  for (int k = 0; k < 2*l+1; k++) {
    int m = k == 0 ? 0 : (k % 2 ? (k+1)/2 : -(k/2));
    if (l == 1) m = k == 0 ? 1 : (k == 1 ? -1 : 0);   // p is x y z everywhere
    int am = std::abs(m), w0 = m < 0 ? 1 : 0;
    vector<Term> t;
    for (int tt = 0; tt <= (l-am)/2; tt++)
      for (int u = 0; u <= tt; u++)
        for (int w = w0; w <= 2*((am - w0)/2) + w0 + (w0 ? 0 : 0) && w <= am; w += 2) {
          double cf = ((tt + (w-w0)/2) % 2 ? -1 : 1) * std::pow(0.25, tt)
                      * binom(l, tt) * binom(l-tt, am+tt) * binom(tt, u) * binom(am, w);
          if (cf == 0) continue;
          Term x = {2*tt+am-2*u-w, 2*u+w, l-2*tt-am, cf};
          t.push_back(x);
        }
    EspBasisFunction f = fromTerms(t, c0);
    f.exponent.push_back(1); f.contraction.push_back(1);
    double s = EspField::overlapOf(f, f);
    for (size_t i = 0; i < t.size(); i++) t[i].w *= std::sqrt(target/s);
    out.push_back(t);
  }
  return out;
}

}  // namespace

EspBasisFunction rawAo(const Shell& s, const double* c, int comp) {
  EspBasisFunction f;
  if (s.cartesian) {
    Pow p = cartCanonical(s.l)[comp];
    vector<Term> t(1);
    t[0].a = p[0]; t[0].b = p[1]; t[0].c = p[2]; t[0].w = 1;
    f = fromTerms(t, c);
  } else {
    f = fromTerms(sphericalTerms(s.l)[comp], c);
  }
  f.exponent = s.exponent;
  f.contraction = s.coef;
  return f;
}

namespace {

bool geometryCentres(const Model& m, int g, vector<double>& xyz, string& err) {
  if (g < 0 || size_t(g) >= m.nframes()) { err = "no such geometry"; return false; }
  xyz.assign(m.coord.begin() + size_t(g)*3*m.natoms(),
             m.coord.begin() + size_t(g+1)*3*m.natoms());
  return true;
}

bool rawAos(const Model& m, int g, vector<EspBasisFunction>& aos, string& err) {
  if (m.basis.type != "GTO") { err = "overlap needs a GTO basis"; return false; }
  vector<double> xyz;
  if (!geometryCentres(m, g, xyz, err)) return false;
  aos.clear();
  for (size_t i = 0; i < m.basis.shells.size(); i++) {
    const Shell& s = m.basis.shells[i];
    if (s.atom < 0 || size_t(s.atom) >= m.natoms()) { err = "shell atom out of range"; return false; }
    for (int k = 0; k < shellSize(s); k++) aos.push_back(rawAo(s, &xyz[3*s.atom], k));
  }
  return true;
}

void rawOverlap(const vector<EspBasisFunction>& a, vector<double>& S) {
  size_t n = a.size();
  S.assign(n*n, 0.0);
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j <= i; j++)
      S[i*n+j] = S[j*n+i] = EspField::overlapOf(a[i], a[j]);
}

}  // namespace

bool buildAos(const Model& m, int g, vector<EspBasisFunction>& aos, string& err) {
  if (!rawAos(m, g, aos, err)) return false;
  for (size_t i = 0; i < aos.size(); i++) {
    double s = std::sqrt(EspField::overlapOf(aos[i], aos[i]));
    for (size_t t = 0; t < aos[i].angularCoef.size(); t++) aos[i].angularCoef[t] /= s;
  }
  return true;
}

bool overlapMatrix(const Model& m, int g, vector<double>& S, string& err) {
  vector<EspBasisFunction> aos;
  if (m.basis.orthonormal) { err = "orthonormal basis: S = 1"; return false; }
  if (!buildAos(m, g, aos, err)) return false;
  rawOverlap(aos, S);
  return true;
}

// max |C S C^T - 1| over MOs with occupation > 0
static double orthoError(const Mos& mo, const vector<double>& S) {
  size_t n = mo.nao;
  double worst = 0;
  vector<size_t> occ;
  for (size_t i = 0; i < mo.nmo; i++)
    if (mo.occupation.empty() || mo.occupation[i] > 1e-8) occ.push_back(i);
  for (size_t a = 0; a < occ.size(); a++) {
    vector<double> sc(n, 0.0);
    const double* ca = &mo.coef[occ[a]*n];
    for (size_t i = 0; i < n; i++) {
      double x = 0;
      for (size_t j = 0; j < n; j++) x += S[i*n+j]*ca[j];
      sc[i] = x;
    }
    for (size_t b = 0; b <= a; b++) {
      const double* cb = &mo.coef[occ[b]*n];
      double x = 0;
      for (size_t i = 0; i < n; i++) x += cb[i]*sc[i];
      worst = std::max(worst, std::fabs(x - (a == b ? 1.0 : 0.0)));
    }
  }
  return worst;
}

bool normaliseSource(Model& m, string& note, string& err) {
  if (m.basis.orthonormal || m.mos.empty()) return true;
  double bestErr = 1e300;
  Model best;
  const char* names[2][2] = {{"raw primitives, unit AOs", "raw primitives, monomial AOs"},
                             {"normalised primitives, unit AOs", "normalised primitives, monomial AOs"}};
  const char* pick = 0;
  for (int prim = 0; prim < 2; prim++) {
    Model t = m;
    if (prim)
      for (size_t i = 0; i < t.basis.shells.size(); i++) {
        Shell& s = t.basis.shells[i];
        for (size_t k = 0; k < s.coef.size(); k++) s.coef[k] *= nrad(s.exponent[k], s.l);
      }
    vector<EspBasisFunction> aos;
    if (!rawAos(t, 0, aos, err)) return false;
    vector<double> S;
    rawOverlap(aos, S);
    size_t n = aos.size();
    vector<double> N(n);
    for (size_t i = 0; i < n; i++) N[i] = std::sqrt(S[i*n+i]);
    vector<double> Su(S);
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < n; j++) Su[i*n+j] /= N[i]*N[j];
    for (int mono = 0; mono < 2; mono++) {
      Model u = t;
      double worst = 0;
      for (size_t k = 0; k < u.mos.size(); k++) {
        Mos& mo = u.mos[k];
        if (mo.nao != n) { err = "MO width does not match the basis"; return false; }
        if (mono)
          for (size_t i = 0; i < mo.nmo; i++)
            for (size_t j = 0; j < n; j++) mo.coef[i*n+j] *= N[j];
        worst = std::max(worst, orthoError(mo, Su));
      }
      if (worst < bestErr) { bestErr = worst; best = u; pick = names[prim][mono]; }
    }
  }
  string flips;
  if (bestErr > 1e-6) {
    // Some programs (ORCA) print individual f functions with the opposite
    // sign.  One sign pattern shared by all spherical f shells is searched.
    vector<size_t> foff;
    size_t off = 0;
    for (size_t k = 0; k < best.basis.shells.size(); k++) {
      const Shell& s = best.basis.shells[k];
      if (s.l == 3 && !s.cartesian) foff.push_back(off);
      off += shellSize(s);
    }
    vector<EspBasisFunction> aos;
    vector<double> S;
    if (!foff.empty() && rawAos(best, 0, aos, err)) {
      rawOverlap(aos, S);
      size_t n = aos.size();
      for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++) if (i != j) S[i*n+j] /= std::sqrt(S[i*n+i]*S[j*n+j]);
      for (size_t i = 0; i < n; i++) S[i*n+i] = 1;
      int bestPat = 0;
      for (int pat = 1; pat < 128; pat++) {
        Model u = best;
        for (size_t k = 0; k < u.mos.size(); k++)
          for (size_t i = 0; i < u.mos[k].nmo; i++)
            for (size_t q = 0; q < foff.size(); q++)
              for (int c = 0; c < 7; c++)
                if (pat >> c & 1) u.mos[k].coef[i*n + foff[q] + c] *= -1;
        double worst = 0;
        for (size_t k = 0; k < u.mos.size(); k++) worst = std::max(worst, orthoError(u.mos[k], S));
        if (worst < bestErr) { bestErr = worst; bestPat = pat; }
      }
      if (bestPat) {
        for (size_t k = 0; k < best.mos.size(); k++)
          for (size_t i = 0; i < best.mos[k].nmo; i++)
            for (size_t q = 0; q < foff.size(); q++)
              for (int c = 0; c < 7; c++)
                if (bestPat >> c & 1) best.mos[k].coef[i*n + foff[q] + c] *= -1;
        flips = "; f sign pattern " + std::to_string(bestPat) + " (bit c = component c, order m=0,+1,-1,+2,-2,+3,-3)";
      }
    }
  }
  m.basis = best.basis;
  m.mos = best.mos;
  char buf[300];
  snprintf(buf, sizeof buf, "source convention: %s (occupied-MO orthonormality error %.1e)%s", pick, bestErr, flips.c_str());
  note = buf;
  return true;
}

// ---- frequencies from a Hessian -------------------------------------------

namespace {
const double kStd[19] = {0,1.008,4.0026,6.94,9.0122,10.81,12.011,14.007,15.999,18.998,20.18,
  22.99,24.305,26.982,28.085,30.974,32.06,35.45,39.948};
const double kIso[19] = {0,1.007825,4.002603,7.016004,9.012182,11.009305,12.0,14.003074,15.994915,
  18.998403,19.992440,22.989770,23.985042,26.981539,27.976927,30.973762,31.972071,34.968853,39.962383};

void jacobi(vector<double>& A, size_t n, vector<double>& ev) {
  for (int sweep = 0; sweep < 100; sweep++) {
    double off = 0;
    for (size_t i = 0; i < n; i++) for (size_t j = 0; j < i; j++) off += A[i*n+j]*A[i*n+j];
    if (off < 1e-26) break;
    for (size_t p = 0; p < n; p++)
      for (size_t q = p+1; q < n; q++) {
        double apq = A[p*n+q];
        if (std::fabs(apq) < 1e-300) continue;
        double th = (A[q*n+q]-A[p*n+p])/(2*apq);
        double t = (th >= 0 ? 1 : -1)/(std::fabs(th)+std::sqrt(th*th+1));
        double c = 1/std::sqrt(t*t+1), s = t*c;
        for (size_t k = 0; k < n; k++) {
          double kp = A[k*n+p], kq = A[k*n+q];
          A[k*n+p] = c*kp - s*kq; A[k*n+q] = s*kp + c*kq;
        }
        for (size_t k = 0; k < n; k++) {
          double pk = A[p*n+k], qk = A[q*n+k];
          A[p*n+k] = c*pk - s*qk; A[q*n+k] = s*pk + c*qk;
        }
      }
  }
  ev.resize(n);
  for (size_t i = 0; i < n; i++) ev[i] = A[i*n+i];
}
}  // namespace

bool frequenciesFromHessian(const Model& m, const Hessian& h, vector<double>& freq) {
  size_t N = m.natoms(), n = 3*N;
  if (N == 0 || h.upper.size() != n*(n+1)/2) return false;
  std::map<string,string>::const_iterator it = m.provenance.find("code");
  const double* mt = (it != m.provenance.end() && it->second.compare(0, 8, "Gaussian") == 0) ? kIso : kStd;
  vector<double> mass(N);
  for (size_t a = 0; a < N; a++) {
    int z = m.atomicNumber[a];
    if (z < 1 || z > 18) return false;
    mass[a] = mt[z];
  }
  vector<double> H(n*n);
  size_t k = 0;
  for (size_t i = 0; i < n; i++)
    for (size_t j = i; j < n; j++, k++) H[i*n+j] = H[j*n+i] = h.upper[k];
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < n; j++) H[i*n+j] /= std::sqrt(mass[i/3]*mass[j/3]);
  // translation/rotation vectors, mass-weighted, orthonormalised
  const double* x = &m.coord[0];
  double com[3] = {0,0,0}, mt_ = 0;
  for (size_t a = 0; a < N; a++) { for (int c = 0; c < 3; c++) com[c] += mass[a]*x[3*a+c]; mt_ += mass[a]; }
  for (int c = 0; c < 3; c++) com[c] /= mt_;
  vector<vector<double> > vs;
  for (int c = 0; c < 3; c++) {
    vector<double> v(n, 0.0);
    for (size_t a = 0; a < N; a++) v[3*a+c] = std::sqrt(mass[a]);
    vs.push_back(v);
  }
  for (int c = 0; c < 3; c++) {
    vector<double> v(n, 0.0);
    for (size_t a = 0; a < N; a++) {
      double r[3] = {x[3*a]-com[0], x[3*a+1]-com[1], x[3*a+2]-com[2]};
      int c1 = (c+1)%3, c2 = (c+2)%3;
      v[3*a+c1] = -r[c2]*std::sqrt(mass[a]);
      v[3*a+c2] =  r[c1]*std::sqrt(mass[a]);
    }
    vs.push_back(v);
  }
  vector<vector<double> > ob;
  for (size_t i = 0; i < vs.size(); i++) {
    vector<double> v = vs[i];
    for (size_t j = 0; j < ob.size(); j++) {
      double d = 0; for (size_t q = 0; q < n; q++) d += v[q]*ob[j][q];
      for (size_t q = 0; q < n; q++) v[q] -= d*ob[j][q];
    }
    double nn = 0; for (size_t q = 0; q < n; q++) nn += v[q]*v[q];
    if (nn < 1e-10) continue;
    nn = std::sqrt(nn);
    for (size_t q = 0; q < n; q++) v[q] /= nn;
    ob.push_back(v);
  }
  vector<double> P(n*n, 0.0);
  for (size_t i = 0; i < n; i++) P[i*n+i] = 1;
  for (size_t j = 0; j < ob.size(); j++)
    for (size_t a = 0; a < n; a++) for (size_t b = 0; b < n; b++) P[a*n+b] -= ob[j][a]*ob[j][b];
  vector<double> T(n*n, 0.0), Hp(n*n, 0.0);
  for (size_t i = 0; i < n; i++) for (size_t j = 0; j < n; j++) {
    double s = 0; for (size_t q = 0; q < n; q++) s += H[i*n+q]*P[q*n+j]; T[i*n+j] = s; }
  for (size_t i = 0; i < n; i++) for (size_t j = 0; j < n; j++) {
    double s = 0; for (size_t q = 0; q < n; q++) s += P[i*n+q]*T[q*n+j];
    Hp[i*n+j] = s + (i == j ? 1.0 : 0.0)*0;   }
  for (size_t j = 0; j < ob.size(); j++)
    for (size_t a = 0; a < n; a++) for (size_t b = 0; b < n; b++) Hp[a*n+b] += 1e3*ob[j][a]*ob[j][b];
  vector<double> ev;
  jacobi(Hp, n, ev);
  std::sort(ev.begin(), ev.end());
  size_t keep = n - ob.size();
  freq.clear();
  for (size_t i = 0; i < keep; i++) {
    double w = std::sqrt(std::fabs(ev[i])/1822.888486209)*219474.6313702;
    freq.push_back(ev[i] < 0 ? -w : w);
  }
  return true;
}

Invariants check(const Model& m) {
  Invariants r;
  char buf[300];
  for (size_t k = 0; k < m.mos.size() && k < 4; k++) {
    const Mos& mo = m.mos[k];
    vector<double> S;
    string err;
    if (m.basis.orthonormal) { r.report += "MOs: orthonormal basis, no overlap check\n"; continue; }
    if (!overlapMatrix(m, mo.geometry, S, err)) { r.report += "MOs: " + err + "\n"; r.ok = false; continue; }
    size_t n = mo.nao;
    double tr = 0, nocc = 0;
    for (size_t i = 0; i < mo.nmo; i++) {
      double o = mo.occupation.empty() ? 0 : mo.occupation[i];
      nocc += o;
      double x = 0;
      for (size_t a = 0; a < n; a++) for (size_t b = 0; b < n; b++)
        x += mo.coef[i*n+a]*S[a*n+b]*mo.coef[i*n+b];
      tr += o*x;
    }
    double oe = orthoError(mo, S);
    r.haveMos = true;
    r.traceErr[mo.spin & 1] = std::fabs(tr - nocc);
    r.orthoErr = std::max(r.orthoErr, oe);
    snprintf(buf, sizeof buf, "MOs spin %d: tr(PS)=%.10f for N=%.4f (err %.2e), max|C'SC-1| over occupied = %.2e\n",
             mo.spin, tr, nocc, std::fabs(tr-nocc), oe);
    r.report += buf;
    if (std::fabs(tr-nocc) > 1e-6 || oe > 1e-6) r.ok = false;
  }
  for (size_t k = 0; k < m.hessian.size(); k++) {
    r.haveHessian = true;
    vector<double> f;
    if (!frequenciesFromHessian(m, m.hessian[k], f)) { r.report += "Hessian: size or element not supported\n"; r.ok = false; continue; }
    snprintf(buf, sizeof buf, "Hessian %d: %zu frequencies from the stored Hessian, lowest %.1f highest %.1f cm-1\n",
             int(k), f.size(), f.empty() ? 0 : f[0], f.empty() ? 0 : f.back());
    r.report += buf;
    for (size_t v = 0; v < m.vib.size(); v++) {
      vector<double> s = m.vib[v].frequency;
      if (s.empty() || m.vib[v].geometry != m.hessian[k].geometry) continue;
      std::sort(s.begin(), s.end());
      size_t cnt = std::min(s.size(), f.size());
      double worst = 0;
      for (size_t i = 0; i < cnt; i++)   // compare from the high end (modes the source omits are the low ones)
        worst = std::max(worst, std::fabs(s[s.size()-1-i] - f[f.size()-1-i]));
      r.haveFreq = true;
      r.freqDiff = worst;
      snprintf(buf, sizeof buf, "  against the code's own %zu frequencies: max difference %.3f cm-1\n", cnt, worst);
      r.report += buf;
      if (worst > 0.5) r.ok = false;
    }
  }
  return r;
}

}  // namespace resultsh5
