// Library tests for FragmentAnalysis (#201): exactness on a synthetic
// fragment set, the refusals, and OPDOS against an independent P.S sum.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "tdat/FragmentAnalysis.H"
#include "tdat/ResultsH5.H"

using namespace resultsh5;
using namespace fragorb;

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s: ", #c); printf(__VA_ARGS__); printf("\n"); } } while (0)

static size_t shellN(const Shell& s) { return s.cartesian ? (s.l + 1) * (s.l + 2) / 2 : 2 * s.l + 1; }

// The atoms [a0, a1) of cx as a model of their own (frame of the MOs only).
static Model subModel(const Model& cx, int a0, int a1) {
  Model f;
  size_t n = cx.natoms(), g = cx.mos[0].geometry;
  f.provenance = cx.provenance;
  for (int a = a0; a < a1; a++) {
    f.atomicNumber.push_back(cx.atomicNumber[a]);
    for (int x = 0; x < 3; x++) f.coord.push_back(cx.coord[(g * n + a) * 3 + x]);
  }
  f.geomEnergy.assign(1, 0.0);
  f.basis = cx.basis;
  f.basis.shells.clear();
  for (size_t k = 0; k < cx.basis.shells.size(); k++) {
    Shell s = cx.basis.shells[k];
    if (s.atom < a0 || s.atom >= a1) continue;
    s.atom -= a0;
    f.basis.shells.push_back(s);
  }
  return f;
}

// Cyclic Jacobi eigenproblem of a symmetric matrix (row-major, n x n).
static void jacobi(vector<double> A, size_t n, vector<double>& w, vector<double>& V) {
  V.assign(n * n, 0.0);
  for (size_t i = 0; i < n; i++) V[i * n + i] = 1;
  for (int sweep = 0; sweep < 60; sweep++) {
    double off = 0;
    for (size_t p = 0; p < n; p++) for (size_t q = p + 1; q < n; q++) off += A[p * n + q] * A[p * n + q];
    if (off < 1e-26) break;
    for (size_t p = 0; p < n; p++)
      for (size_t q = p + 1; q < n; q++) {
        double apq = A[p * n + q];
        if (std::fabs(apq) < 1e-300) continue;
        double th = (A[q * n + q] - A[p * n + p]) / (2 * apq);
        double t = (th >= 0 ? 1.0 : -1.0) / (std::fabs(th) + std::sqrt(th * th + 1));
        double c = 1 / std::sqrt(t * t + 1), s = t * c;
        for (size_t k = 0; k < n; k++) {
          double akp = A[k * n + p], akq = A[k * n + q];
          A[k * n + p] = c * akp - s * akq;
          A[k * n + q] = s * akp + c * akq;
        }
        for (size_t k = 0; k < n; k++) {
          double apk = A[p * n + k], aqk = A[q * n + k];
          A[p * n + k] = c * apk - s * aqk;
          A[q * n + k] = s * apk + c * aqk;
        }
        for (size_t k = 0; k < n; k++) {
          double vkp = V[k * n + p], vkq = V[k * n + q];
          V[k * n + p] = c * vkp - s * vkq;
          V[k * n + q] = s * vkp + c * vkq;
        }
      }
  }
  w.resize(n);
  for (size_t i = 0; i < n; i++) w[i] = A[i * n + i];
}

// Fragment "MOs": Loewdin-orthonormalised AOs, a complete orthonormal set
// spanning the fragment's basis, ordered by increasing index as energy.
static void loewdinMos(Model& f) {
  vector<double> S, w, V;
  string err;
  if (!overlapMatrix(f, 0, S, err)) { printf("overlap: %s\n", err.c_str()); exit(1); }
  size_t n = f.basis.nao();
  jacobi(S, n, w, V);
  Mos m;
  m.nmo = m.nao = n;
  m.coef.assign(n * n, 0.0);
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < n; j++) {
      double s = 0;
      for (size_t k = 0; k < n; k++) s += V[i * n + k] * V[j * n + k] / std::sqrt(w[k]);
      m.coef[i * n + j] = s;
    }
  for (size_t i = 0; i < n; i++) { m.energy.push_back(double(i)); m.occupation.push_back(0); }
  f.mos.push_back(m);
}

int main(int argc, char** argv) {
  if (argc < 2) { printf("usage: fragorb_test crco6/results.h5\n"); return 2; }
  Model cx;
  string err;
  if (!readFile(argv[1], cx, err)) { printf("read: %s\n", err.c_str()); return 2; }
  size_t nao = cx.basis.nao();
  Model cr = subModel(cx, 0, 1), co6 = subModel(cx, 1, 13);
  loewdinMos(cr);
  loewdinMos(co6);
  vector<NamedModel> fr(2);
  fr[0].name = "Cr"; fr[0].model = &cr;
  fr[1].name = "(CO)6"; fr[1].model = &co6;
  Options opt;
  Analysis a;
  CHECK(analyse(cx, fr, opt, a, err), "%s", err.c_str());
  if (fails) return 1;

  // compositions sum to 1; the expansion reproduces every MO coefficient
  const Channel& ch = a.channel[0];
  double worstSum = 0, worstScpa = 0, worstRem = 0, worstRec = 0;
  size_t nCr = cr.basis.nao();
  for (size_t i = 0; i < ch.nmo; i++) {
    double sm = 0, ss = 0;
    for (size_t k = 0; k < ch.fo.size(); k++) { sm += ch.mulliken[k * ch.nmo + i]; ss += ch.scpa[k * ch.nmo + i]; }
    worstSum = std::max(worstSum, std::fabs(sm - 1));
    worstScpa = std::max(worstScpa, std::fabs(ss - 1));
    worstRem = std::max(worstRem, std::fabs(ch.remainder[i]));
    for (size_t p = 0; p < nao; p++) {
      double v = 0;
      for (size_t k = 0; k < ch.fo.size(); k++) {
        const Fo& fo = ch.fo[k];
        const Mos& fm = fo.fragment == 0 ? cr.mos[0] : co6.mos[0];
        size_t off = fo.fragment == 0 ? 0 : nCr, fnao = fm.nao;
        if (p < off || p >= off + fnao) continue;
        v += ch.coef[k * ch.nmo + i] * fm.coef[(fo.index - 1) * fnao + (p - off)];
      }
      worstRec = std::max(worstRec, std::fabs(v - cx.mos[0].coef[i * nao + p]));
    }
  }
  printf("synthetic: max |sum Mulliken - 1| %.2e, SCPA %.2e, remainder %.2e, coefficient reconstruction %.2e\n",
         worstSum, worstScpa, worstRem, worstRec);
  CHECK(worstSum < 1e-8 && worstScpa < 1e-8 && worstRem < 1e-8 && worstRec < 1e-8, "exactness");
  // a MO that is one AO of Cr's set is 100 % Cr FOs
  double crShare = 0;
  for (size_t k = 0; k < ch.fo.size(); k++) if (ch.fo[k].fragment == 0) crShare += ch.mulliken[k * ch.nmo + 0];
  CHECK(crShare > 0.99, "MO 1 is the Cr 1s core: Cr share %.4f", crShare);

  // seven fragments (Cr and six CO) give the same sums
  vector<Model> co(6);
  vector<NamedModel> f7(1, fr[0]);
  for (int k = 0; k < 6; k++) {
    co[k] = subModel(cx, 1 + 2 * k, 3 + 2 * k);
    loewdinMos(co[k]);
    NamedModel nm; nm.name = "CO" + std::to_string(k + 1); nm.model = &co[k];
    f7.push_back(nm);
  }
  Analysis a7;
  CHECK(analyse(cx, f7, opt, a7, err), "%s", err.c_str());
  double w7 = 0;
  for (size_t i = 0; i < a7.channel[0].nmo; i++) {
    double s = 0;
    for (size_t k = 0; k < a7.channel[0].fo.size(); k++) s += a7.channel[0].mulliken[k * a7.channel[0].nmo + i];
    w7 = std::max(w7, std::fabs(s - 1));
  }
  CHECK(w7 < 1e-8, "seven fragments: %.2e", w7);

  // an unrestricted complex: two channels, restricted fragments serve both
  Model ux = cx;
  ux.mos.push_back(ux.mos[0]);
  ux.mos[1].spin = 1;
  Analysis au;
  CHECK(analyse(ux, fr, opt, au, err) && au.channel.size() == 2 && !au.restricted, "unrestricted: %s", err.c_str());

  // refusals
  Model moved = cr;
  moved.coord[0] += 0.02;
  vector<NamedModel> bad = fr;
  NamedModel mv = fr[0]; mv.model = &moved; bad[0] = mv;
  Analysis x;
  CHECK(!analyse(cx, bad, opt, x, err) && err.find("matches no complex atom") != string::npos, "moved atom: %s", err.c_str());
  Model e1 = subModel(cx, 0, 7), e2 = subModel(cx, 5, 13);
  loewdinMos(e1); loewdinMos(e2);
  vector<NamedModel> ov(2);
  ov[0].name = "A"; ov[0].model = &e1; ov[1].name = "B"; ov[1].model = &e2;
  CHECK(!analyse(cx, ov, opt, x, err) && err.find("overlap") != string::npos, "overlapping fragments: %s", err.c_str());
  vector<NamedModel> only(1, fr[1]);
  CHECK(!analyse(cx, only, opt, x, err) && err.find("do not cover") != string::npos, "uncovered: %s", err.c_str());
  Options un = opt; un.allowUncovered = true;
  CHECK(analyse(cx, only, un, x, err), "allow-uncovered: %s", err.c_str());
  double maxRem = 0;
  for (size_t i = 0; i < x.channel[0].nmo; i++) maxRem = std::max(maxRem, x.channel[0].remainder[i]);
  CHECK(maxRem > 0.5, "Cr 1s alone must leave a large remainder (%.3f)", maxRem);
  Model badBasis = co6;
  badBasis.basis.shells[0].exponent[0] *= 1.1;
  NamedModel bb = fr[1]; bb.model = &badBasis;
  vector<NamedModel> bbv(2); bbv[0] = fr[0]; bbv[1] = bb;
  CHECK(!analyse(cx, bbv, opt, x, err) && err.find("basis") != string::npos, "basis mismatch: %s", err.c_str());

  // DOS: gross populations sum to 1, integral of the curve counts the MOs in
  // the window, and the occupied overlap equals 2 sum P_mn S_mn from P directly.
  DosOptions dopt;
  AtomSet sa, sb;
  sa.name = "Cr"; sa.atoms.push_back(0);
  sb.name = "CO6";
  for (int k = 1; k < 13; k++) sb.atoms.push_back(k);
  dopt.sets.push_back(sa); dopt.sets.push_back(sb);
  dopt.emin = -40; dopt.emax = 30; dopt.step = 0.02;
  DosResult d;
  CHECK(dos(cx, dopt, d, err), "%s", err.c_str());
  double gsum = 0;
  for (size_t i = 0; i < d.moEnergy.size(); i++) gsum = std::max(gsum, std::fabs(d.moGross[i * 2] + d.moGross[i * 2 + 1] - 1));
  int inWin = 0;
  for (size_t i = 0; i < d.moEnergy.size(); i++) if (d.moEnergy[i] > -39 && d.moEnergy[i] < 29) inWin++;
  double integral = 0;
  for (size_t g = 0; g < d.energy.size(); g++) integral += d.tdos[g] * dopt.step;
  vector<double> S;
  CHECK(overlapMatrix(cx, 0, S, err), "%s", err.c_str());
  double op = 0;
  const Mos& m = cx.mos[0];
  size_t nCrAo = cr.basis.nao();
  for (size_t p = 0; p < nCrAo; p++)
    for (size_t q = nCrAo; q < nao; q++) {
      double P = 0;
      for (size_t i = 0; i < m.nmo; i++) P += m.occupation[i] * m.coef[i * nao + p] * m.coef[i * nao + q];
      op += 2 * P * S[p * nao + q];
    }
  printf("dos: sum gross-1 %.2e, TDOS integral %.4f for %d MOs x2, occupied OP(Cr,CO6) %.6f, from P.S %.6f\n",
         gsum, integral, inWin, d.occupiedOverlap[0], op);
  CHECK(gsum < 1e-8, "gross sum");
  CHECK(std::fabs(integral - 2 * inWin) < 0.01, "TDOS integral");
  CHECK(std::fabs(d.occupiedOverlap[0] - op) < 1e-8, "OP vs P.S");
  CHECK(d.occupiedOverlap[0] > 0, "Cr-CO bonding overlap population should be positive: %f", d.occupiedOverlap[0]);
  printf(fails ? "RESULT FAIL\n" : "RESULT OK\n");
  return fails ? 1 : 0;
}
