//  Timing and accuracy harness for the exact ESP evaluation, on a
//  benzene-shaped problem built in code (6-31G*-like basis, a smooth
//  synthetic density matrix) so it needs no calculation and no build
//  tree.  The synthetic density is not a wavefunction; it has the same
//  pair structure, which is what decides the cost.
//
//    benchEspField ref|fast|fastmt N out.bin [threads]
//
//  ref  = EspField::potential() point by point (the reference)
//  fast = EspField::Prepared, one thread; fastmt = several.
//  out.bin gets N^3 doubles; compare two of them with
//  tests/slater/compareGrids.py.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include "tdat/EspField.H"
using namespace std;

static const double PI = 3.14159265358979323846;

static void addShell(vector<EspBasisFunction>& basis, const double* ctr,
                     int l, const vector<double>& e, const vector<double>& c)
{
  typedef array<double,4> T;   // coef, px, py, pz
  vector<vector<T> > forms;
  if (l == 0) forms = { {T{{1,0,0,0}}} };
  else if (l == 1) forms = { {T{{1,1,0,0}}}, {T{{1,0,1,0}}}, {T{{1,0,0,1}}} };
  else forms = { {T{{2,0,0,2}},T{{-1,2,0,0}},T{{-1,0,2,0}}}, {T{{1,1,0,1}}},
                 {T{{1,0,1,1}}}, {T{{1,1,1,0}}}, {T{{1,2,0,0}},T{{-1,0,2,0}}} };
  for (size_t f = 0; f < forms.size(); f++) {
    EspBasisFunction b;
    for (int k = 0; k < 3; k++) b.center[k] = ctr[k];
    for (size_t q = 0; q < forms[f].size(); q++) {
      const T& t = forms[f][q];
      b.angularCoef.push_back(t[0]); b.powerX.push_back((int)t[1]);
      b.powerY.push_back((int)t[2]); b.powerZ.push_back((int)t[3]);
    }
    for (size_t i = 0; i < e.size(); i++) {
      b.exponent.push_back(e[i]);
      b.contraction.push_back(c[i]*pow(2*e[i]/PI,0.75)*pow(4*e[i],l/2.0));
    }
    double s = EspField::overlapOf(b, b);
    for (size_t i = 0; i < b.contraction.size(); i++) b.contraction[i] /= sqrt(s);
    basis.push_back(b);
  }
}

int main(int argc, char** argv)
{
  if (argc < 4) return 2;
  string mode = argv[1];
  const int N = atoi(argv[2]);
  const int nthreads = argc > 4 ? atoi(argv[4]) : 4;
  const double rC = 2.6443, rH = 4.6975;

  vector<EspBasisFunction> basis;
  vector<EspNucleus> nuclei;
  for (int a = 0; a < 6; a++) {
    double ang = a*PI/3;
    double C3[3] = {rC*cos(ang), rC*sin(ang), 0};
    double H3[3] = {rH*cos(ang), rH*sin(ang), 0};
    EspNucleus n; memcpy(n.center, C3, sizeof C3); n.charge = 6; nuclei.push_back(n);
    n.charge = 1; memcpy(n.center, H3, sizeof H3); nuclei.push_back(n);
    addShell(basis, C3, 0, {3047.5249,457.36951,103.94869,29.210155,9.2866630,3.1639270},
             {0.0018347,0.0140373,0.0688426,0.2321844,0.4679413,0.3623120});
    addShell(basis, C3, 0, {7.8682724,1.8812885,0.5442493}, {-0.1193324,-0.1608542,1.1434564});
    addShell(basis, C3, 1, {7.8682724,1.8812885,0.5442493}, {0.0689991,0.3164240,0.7443083});
    addShell(basis, C3, 0, {0.1687144}, {1.0});
    addShell(basis, C3, 1, {0.1687144}, {1.0});
    addShell(basis, C3, 2, {0.8}, {1.0});
    addShell(basis, H3, 0, {18.7311370,2.8253937,0.6401217}, {0.03349460,0.23472695,0.81375733});
    addShell(basis, H3, 0, {0.1612778}, {1.0});
  }
  const size_t nb = basis.size();

  //  Synthetic density: 21 doubly occupied "orbitals", each centred on
  //  one atom and decaying with distance from it.
  srand(12345);
  vector<double> P(nb*nb, 0.0);
  for (int m = 0; m < 21; m++) {
    const double* c0 = nuclei[(m*7)%nuclei.size()].center;
    vector<double> c(nb);
    for (size_t u = 0; u < nb; u++) {
      double d2 = 0;
      for (int k = 0; k < 3; k++) { double d = basis[u].center[k]-c0[k]; d2 += d*d; }
      c[u] = (rand()/(double)RAND_MAX - 0.5)*exp(-0.08*d2);
    }
    for (size_t u = 0; u < nb; u++) for (size_t v = 0; v < nb; v++) P[u*nb+v] += 2*c[u]*c[v];
  }
  EspField::Pairs pairs;
  EspField::selectPairs(basis, P, 1.0e-8, pairs);
  fprintf(stderr, "basis %zu, pairs %zu, grid %d^3\n", nb, pairs.size(), N);

  vector<double> out((size_t)N*N*N);
  auto point = [&](size_t idx, double* p) {
    int i = idx % N, j = (idx/N) % N, k = idx/((size_t)N*N);
    p[0] = -9.0 + 18.0*i/(N-1); p[1] = -9.0 + 18.0*j/(N-1);
    p[2] = -6.0 + 12.0*k/(N-1);
  };

  auto t0 = chrono::steady_clock::now();
  if (mode == "ref") {
    for (size_t idx = 0; idx < out.size(); idx++) {
      double p[3]; point(idx, p);
      out[idx] = EspField::potential(basis, pairs, nuclei, p);
    }
  } else {
    EspField::Prepared prep;
    EspField::prepare(basis, pairs, prep);
    fprintf(stderr, "prepared: %zu merged terms\n", prep.size());
    double tp = chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    fprintf(stderr, "prepare %.3f s\n", tp);
    int nt = (mode == "fastmt") ? nthreads : 1;
    atomic<size_t> next(0);
    auto work = [&]() {
      for (;;) {
        size_t s = next.fetch_add(64);
        if (s >= out.size()) return;
        for (size_t idx = s; idx < min(s+64, out.size()); idx++) {
          double p[3]; point(idx, p);
          out[idx] = EspField::potential(prep, nuclei, p);
        }
      }
    };
    vector<thread> th;
    for (int t = 0; t < nt; t++) th.emplace_back(work);
    for (size_t t = 0; t < th.size(); t++) th[t].join();
  }
  double dt = chrono::duration<double>(chrono::steady_clock::now()-t0).count();
  fprintf(stderr, "%s: %.3f s, %.3f ms/point\n", mode.c_str(), dt, 1e3*dt/out.size());
  FILE* f = fopen(argv[3], "wb");
  fwrite(out.data(), sizeof(double), out.size(), f);
  fclose(f);
  return 0;
}
