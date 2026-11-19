// Solvent placement (#1118): counts, no clash below the cut-off, water
// density near 1 g/cm3.  Usage: solvate_test <ECCE source dir>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/Solvate.H"
#include "tdat/TPerTab.H"

using namespace solvate;
using std::string;
using std::vector;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static void cube(Cell c, double a) {
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) c[i][j] = i == j ? a : 0.0;
}

// Smallest d/(r1+r2) by minimum image between atoms of different molecules
// (solvent-solvent) and between solvent and solute.
static void closest(const vector<string>& us, const vector<Vec>& ux,
                    const Result& res, int molSize, const Cell cell,
                    double& solv, double& solu) {
  TPerTab tpt;
  solv = solu = 1.0e9;
  auto r = [&](const string& s) { return tpt.vwr(tpt.atomicNumber(s)); };
  auto mic = [&](const Vec& a, const Vec& b) {
    // orthorhombic cells only in this test
    double s = 0;
    for (int k = 0; k < 3; ++k) {
      double d = a[k] - b[k];
      d -= cell[k][k] * std::round(d / cell[k][k]);
      s += d * d;
    }
    return std::sqrt(s);
  };
  for (size_t i = 0; i < res.xyz.size(); ++i) {
    for (size_t j = i + 1; j < res.xyz.size(); ++j) {
      if ((int)(i / molSize) == (int)(j / molSize)) continue;
      double q = mic(res.xyz[i], res.xyz[j]) / (r(res.symbols[i]) + r(res.symbols[j]));
      if (q < solv) solv = q;
    }
    for (size_t j = 0; j < ux.size(); ++j) {
      double q = mic(res.xyz[i], ux[j]) / (r(res.symbols[i]) + r(us[j]));
      if (q < solu) solu = q;
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) { fprintf(stderr, "usage: solvate_test SRCDIR\n"); return 2; }
  const string dir = string(argv[1]) + "/data/client/solvents";
  vector<string> names = solventNames(dir);
  CHECK(names.size() >= 2, "solvent boxes found: %d", (int)names.size());

  SolventBox w;
  string err;
  CHECK(readSolventBox(dir + "/water216.xyz", w, &err), "%s", err.c_str());
  CHECK(w.molSize == 3 && w.molecules() == 216 && isWater(w),
        "water216: %d molecules of %d", w.molecules(), w.molSize);
  Options opt;

  // 1. Two template edges per side: every copy fits, 8 x 216 molecules.
  {
    Cell c;
    cube(c, 2.0 * w.edge[0]);
    Result res;
    CHECK(fill({}, {}, c, w, opt, res, &err), "%s", err.c_str());
    CHECK(res.molecules == 8 * 216, "whole multiple: %d molecules", res.molecules);
    double rho = density(res.symbols, c);
    printf("whole multiple: %d molecules, density %.4f g/cm3\n", res.molecules, rho);
    CHECK(std::fabs(rho - 1.0) < 0.03, "density %.4f", rho);
  }

  // 2. A box that is no multiple of the template: the seam is cleaned.
  {
    Cell c;
    cube(c, 25.0);
    Result res;
    CHECK(fill({}, {}, c, w, opt, res, &err), "%s", err.c_str());
    double rho = density(res.symbols, c), solv, solu;
    closest({}, {}, res, 3, c, solv, solu);
    printf("25 A cube: %d molecules, density %.4f g/cm3, closest solvent pair %.3f x (r1+r2)\n",
           res.molecules, rho, solv);
    CHECK(rho > 0.90 && rho < 1.05, "density %.4f", rho);
    CHECK(solv >= opt.solventScale - 1.0e-9, "solvent clash %.3f", solv);
  }

  // 3. Around a solute with a margin: no solvent inside the cut-off.
  {
    vector<string> us = {"C", "H", "H", "H", "Cl"};
    vector<Vec> ux = {{{0, 0, 0}}, {{0.63, 0.63, 0.63}}, {{-0.63, -0.63, 0.63}},
                      {{-0.63, 0.63, -0.63}}, {{1.03, -1.03, -1.03}}};
    Cell c;
    Vec shift;
    marginCell(ux, 8.0, c, shift);
    for (Vec& p : ux)
      for (int k = 0; k < 3; ++k) p[k] += shift[k];
    Result res;
    CHECK(fill(us, ux, c, w, opt, res, &err), "%s", err.c_str());
    double solv, solu;
    closest(us, ux, res, 3, c, solv, solu);
    vector<string> all = us;
    all.insert(all.end(), res.symbols.begin(), res.symbols.end());
    printf("CH3Cl, 8 A margin: cell %.2f %.2f %.2f, %d molecules, closest to solute %.3f x (r1+r2), density %.4f\n",
           c[0][0], c[1][1], c[2][2], res.molecules, solu, density(all, c));
    CHECK(res.molecules > 50, "molecules %d", res.molecules);
    CHECK(solu >= opt.scale - 1.0e-9, "solute clash %.3f", solu);
    CHECK((int)res.xyz.size() == 3 * res.molecules, "atoms %d", (int)res.xyz.size());
  }

  // 4. Limits are refused, not filled.
  {
    Cell c;
    cube(c, 60.0);
    Options small = opt;
    small.maxAtoms = 1000;
    Result res;
    CHECK(!fill({}, {}, c, w, small, res, &err), "maxAtoms not enforced");
    Cell flat;
    cube(flat, 10.0);
    flat[2][2] = 0.0;
    CHECK(!fill({}, {}, flat, w, opt, res, &err), "flat cell accepted");
  }

  printf(failures ? "solvate_test: %d failure(s)\n" : "solvate_test: all passed\n", failures);
  return failures ? 1 : 0;
}
