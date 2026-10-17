// Unit checks of FragmentOrbitals: STO overlaps, EHT energies against
// YAeHMOP, automatic fragments, and the projection on a spanning FO set.
//   testFragorb REFDIR           the checks (REFDIR = tests/fragorb/reference)
//   testFragorb --energies X.xyz CHARGE      EHT energies (eV) of a molecule
//   testFragorb --table X.xyz CHARGE         composition table, both FO sets
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "tdat/FoDiagram.H"
#include "tdat/FragmentOrbitals.H"

using namespace std;

static int fails = 0;
static void check(bool ok, const char* what, double value = 0.0)
{
  printf("  %-62s %s (%g)\n", what, ok ? "ok" : "FAIL", value);
  if (!ok) fails++;
}

struct Mol { vector<string> sym; vector<double> xyz; int charge; };

static Mol readXyz(const char* path, int charge)
{
  Mol m; m.charge = charge;
  FILE* f = fopen(path, "r");
  if (!f) { fprintf(stderr, "cannot read %s\n", path); exit(2); }
  int n; if (fscanf(f, "%d", &n) != 1) exit(2);
  char line[256]; fgets(line, sizeof line, f); fgets(line, sizeof line, f);
  for (int i = 0; i < n; i++) {
    char s[8]; double x, y, z;
    if (fscanf(f, "%7s %lf %lf %lf", s, &x, &y, &z) != 4) exit(2);
    m.sym.push_back(s); m.xyz.push_back(x); m.xyz.push_back(y); m.xyz.push_back(z);
  }
  fclose(f);
  return m;
}

// One fragment holding every atom: the plain EHT of the molecule.
static bool wholeMolecule(const Mol& m, FragmentOrbitals::FoSet& set, string& why)
{
  FragmentOrbitals::Fragment f;
  f.name = "all"; f.kind = FragmentOrbitals::LIGAND; f.charge = m.charge;
  for (size_t i = 0; i < m.sym.size(); i++) f.atoms.push_back((int)i);
  vector<FragmentOrbitals::Fragment> v(1, f);
  return FragmentOrbitals::build(m.sym, m.xyz, v, FragmentOrbitals::PER_LIGAND,
                                 set, why);
}


static vector<double> readEht(const string& path)
{
  vector<double> e;
  FILE* f = fopen(path.c_str(), "r");
  if (!f) return e;
  int i; double v;
  while (fscanf(f, "%d %lf", &i, &v) == 2) e.push_back(v);
  fclose(f);
  return e;
}

// EHT of the whole molecule against YAeHMOP's bind (same parameters, weighted
// Wolfsberg-Helmholz, exact Slater overlaps).
static void againstYaehmop(const string& dir, const char* name, int charge)
{
  Mol m = readXyz((dir + "/" + name + ".xyz").c_str(), charge);
  FragmentOrbitals::FoSet set; string why;
  if (!wholeMolecule(m, set, why)) { check(false, why.c_str()); return; }
  const vector<double> ref = readEht(dir + "/" + name + ".eht");
  char b[96];
  snprintf(b, sizeof b, "%s: %zu orbitals, as many as YAeHMOP", name, set.fos.size());
  check(ref.size() == set.fos.size(), b);
  if (ref.size() != set.fos.size()) return;
  double bound = 0, all = 0;
  for (size_t k = 0; k < ref.size(); k++) {
    const double d = fabs(set.fos[k].energy*27.211386245988 - ref[k]);
    all = max(all, d);
    if (ref[k] < 0) bound = max(bound, d);
  }
  snprintf(b, sizeof b, "%s: occupied/bound orbitals within 0.05 eV of YAeHMOP", name);
  check(bound < 0.05, b, bound);
  snprintf(b, sizeof b, "%s: every orbital within 0.5 eV of YAeHMOP", name);
  check(all < 0.5, b, all);
}

static void fragments()
{
  printf("automatic fragments\n");
  struct Case { const char* name; int charge; int metals; int ligands; int metalCharge; };
  const Case cases[] = { {"crco6", 0, 1, 6, 0}, {"conh36", 3, 1, 6, 3}, {"tif6", -2, 1, 6, 4} };
  const string dir = getenv("FRAGORB_REF");
  for (size_t c = 0; c < 3; c++) {
    Mol m = readXyz((dir + "/" + cases[c].name + ".xyz").c_str(), cases[c].charge);
    vector<FragmentOrbitals::Fragment> f; string why;
    bool ok = FragmentOrbitals::autoFragments(m.sym, m.xyz, m.charge, 1, f, why);
    check(ok, cases[c].name);
    if (!ok) continue;
    int nm = 0, nl = 0, ns = 0, charge = 0, atoms = 0;
    for (size_t i = 0; i < f.size(); i++) {
      if (f[i].kind == FragmentOrbitals::METAL) { nm++; if (nm == 1) charge = f[i].charge; }
      if (f[i].kind == FragmentOrbitals::LIGAND) { nl++; atoms += (int)f[i].atoms.size(); }
      if (f[i].kind == FragmentOrbitals::SHELL) ns++;
    }
    char b[96];
    snprintf(b, sizeof b, "%s: %d metal, %d ligands, one shell, metal charge %d",
             cases[c].name, cases[c].metals, cases[c].ligands, cases[c].metalCharge);
    check(nm == cases[c].metals && nl == cases[c].ligands && ns == 1 &&
          charge == cases[c].metalCharge && atoms + 1 == (int)m.sym.size(), b, charge);
  }
  // no metal: not applicable
  Mol w; w.charge = 0; w.sym = { "O", "H", "H" };
  w.xyz = { 0, 0, 0.117, 0, 0.757, -0.469, 0, -0.757, -0.469 };
  vector<FragmentOrbitals::Fragment> f; string why;
  check(!FragmentOrbitals::autoFragments(w.sym, w.xyz, 0, 1, f, why) && !why.empty(),
        "water: not applicable (no metal)");
  string missing;
  vector<string> kr(1, "Kr");
  check(!FragmentOrbitals::canSolve(kr, missing) && missing == "Kr",
        "Kr has no parameters and is declined");
}

// Parent = the whole molecule's own EHT in the same STO functions: the FO set
// then spans the parent space, so capture must be 1 for every MO.
static void composeSelf(const char* xyz, int charge, const char* name, bool print)
{
  Mol m = readXyz(xyz, charge);
  FragmentOrbitals::FoSet whole; string why;
  if (!wholeMolecule(m, whole, why)) { check(false, why.c_str()); return; }
  vector<EspBasisFunction> pb;
  for (size_t i = 0; i < whole.funcs.size(); i++) pb.push_back(whole.funcs[i].fn);
  vector< vector<double> > rows;
  for (size_t k = 0; k < whole.fos.size(); k++) rows.push_back(whole.fos[k].coef);

  vector<FragmentOrbitals::Fragment> frags;
  int total = m.charge;
  if (!FragmentOrbitals::autoFragments(m.sym, m.xyz, total, 1, frags, why)) {
    check(false, why.c_str()); return;
  }
  for (int opt = 0; opt < 2; opt++) {
    const FragmentOrbitals::LigandSet ls =
        opt ? FragmentOrbitals::PER_LIGAND : FragmentOrbitals::LIGAND_SHELL;
    FragmentOrbitals::FoSet set;
    if (!FragmentOrbitals::build(m.sym, m.xyz, frags, ls, set, why)) {
      check(false, why.c_str()); return;
    }
    vector<FragmentOrbitals::MoShares> sh;
    if (!FragmentOrbitals::compose(set, pb, whole.overlap, rows, sh, why)) {
      check(false, why.c_str()); return;
    }
    double worst = 0, wn = 0;
    for (size_t k = 0; k < sh.size(); k++) {
      worst = max(worst, fabs(sh[k].capture - 1.0));
      wn = max(wn, fabs(sh[k].norm - 1.0));
    }
    char b[96];
    snprintf(b, sizeof b, "%s %s: full FO set captures every MO", name,
             opt ? "per-ligand" : "ligand shell");
    check(worst < 1e-7 && wn < 1e-7, b, worst);
    if (print) {
      printf("%s %s\n  MO   E(eV)   occ  class   capture metal  ligand  top\n", name,
             opt ? "per-ligand" : "ligand shell");
      for (size_t k = 0; k < sh.size(); k++) {
        printf("  %3zu %8.3f %4.1f  %-6s  %.3f  %.3f  %.3f ", k + 1,
               whole.fos[k].energy*27.2114, whole.fos[k].occupancy,
               sh[k].klass.c_str(), sh[k].capture, sh[k].metal, sh[k].ligand);
        for (size_t t = 0; t < sh[k].top.size() && t < 3; t++)
          printf(" %s %.0f%%", sh[k].top[t].label.c_str(), 100*sh[k].top[t].share);
        printf("\n");
      }
    }
  }
}

//  The heading formula of a complex, ligands grouped.
static void formulas(const string& dir)
{
  printf("the heading formula\n");
  auto want = [](const Mol& m, const char* text) {
    const string got = FoDiagram::complexFormula(m.sym, m.xyz, m.charge);
    char b[96];
    snprintf(b, sizeof b, "%s (got %s)", text, got.c_str());
    check(got == text, b);
  };
  want(readXyz((dir + "/crco6.xyz").c_str(), 0), "Cr(CO)6");
  want(readXyz((dir + "/conh36.xyz").c_str(), 3), "[Co(NH3)6]^3+");
  want(readXyz((dir + "/tif6.xyz").c_str(), -2), "[TiF6]^2-");

  const double ax[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
  {  // [Co(H2O)6]2+, hydrogens off the axis
    Mol m; m.charge = 2; m.sym.push_back("Co"); m.xyz = { 0, 0, 0 };
    for (int i = 0; i < 6; i++) {
      const double* u = ax[i];
      const double p[3] = { 2.1*u[0], 2.1*u[1], 2.1*u[2] };
      m.sym.push_back("O"); m.xyz.insert(m.xyz.end(), p, p + 3);
      const double v[3] = { u[1] + u[2], u[2] + u[0], u[0] + u[1] };   // perpendicular
      for (int s = -1; s <= 1; s += 2) {
        m.sym.push_back("H");
        for (int k = 0; k < 3; k++) m.xyz.push_back(p[k] + 0.6*s*v[k] + 0.6*u[k]);
      }
    }
    want(m, "[Co(H2O)6]^2+");
  }
  {  // PtCl2(NH3)2, square planar, cis
    Mol m; m.charge = 0; m.sym = { "Pt", "Cl", "Cl", "N", "N" };
    m.xyz = { 0,0,0,  2.3,0,0, 0,2.3,0, -2.05,0,0, 0,-2.05,0 };
    for (int a = 3; a <= 4; a++) {
      const double u[3] = { m.xyz[3*a]/2.05, m.xyz[3*a+1]/2.05, 0 };
      for (int h = 0; h < 3; h++) {
        const double ang = 2.0943951*h, c = cos(ang), s = sin(ang);
        m.sym.push_back("H");
        m.xyz.push_back(m.xyz[3*a] + 0.5*u[0] + 0.9*c*(u[1] != 0 ? 1 : 0));
        m.xyz.push_back(m.xyz[3*a+1] + 0.5*u[1] + 0.9*c*(u[0] != 0 ? 1 : 0));
        m.xyz.push_back(0.9*s);
      }
    }
    want(m, "PtCl2(NH3)2");
  }
  {  // [Fe(CN)6]3-
    Mol m; m.charge = -3; m.sym.push_back("Fe"); m.xyz = { 0, 0, 0 };
    for (int i = 0; i < 6; i++) {
      m.sym.push_back("C"); m.sym.push_back("N");
      for (int k = 0; k < 3; k++) m.xyz.push_back(1.92*ax[i][k]);
      for (int k = 0; k < 3; k++) m.xyz.push_back(3.08*ax[i][k]);
    }
    want(m, "[Fe(CN)6]^3-");
  }
}

//  Orbitals are conserved: per irrep, drawn bonding = drawn antibonding.
static void pairCounts()
{
  printf("the bonding/antibonding count\n");
  typedef FoDiagram::PairLevel PL;
  vector<PL> lv;
  auto add = [&](const char* irrep, int deg, MoLevel::Character c, int shown) {
    PL l; l.irrep = irrep; l.degeneracy = deg; l.character = c; l.shown = shown;
    lv.push_back(l);
  };
  add("T2g", 3, MoLevel::BONDING, 1);
  add("T2g", 3, MoLevel::ANTIBONDING, 1);
  add("T1u", 3, MoLevel::BONDING, -1);       // hidden by the filter
  add("T1u", 3, MoLevel::ANTIBONDING, 1);
  add("A1g", 1, MoLevel::BONDING, 1);
  add("A1g", 1, MoLevel::ANTIBONDING, 0);    // beyond the cut-off
  add("Eg", 2, MoLevel::NONBONDING, 1);
  vector<FoDiagram::PairCount> c = FoDiagram::countPairs(lv);
  map<string, FoDiagram::PairCount> by;
  for (size_t i = 0; i < c.size(); i++) by[c[i].irrep] = c[i];
  check(by["T2g"].ok && by["T2g"].bonding == 3 && by["T2g"].antibonding == 3,
        "a complete pair counts by degeneracy");
  check(!by["T1u"].ok && by["T1u"].message ==
        "T1u: 0 bonding, 3 antibonding: pairs incomplete; 3 partner orbitals "
        "are hidden (tick Show all levels)", "a hidden bonding level is a mismatch, said why");
  check(!by["A1g"].ok && by["A1g"].message.find("cut-off") != string::npos,
        "a partner past the cut-off is a mismatch, said why");
  check(by["Eg"].ok && by["Eg"].nonbonding == 2, "non-bonding levels do not count");
}

int main(int argc, char** argv)
{
  if (argc >= 3 && !strcmp(argv[1], "--table")) {
    composeSelf(argv[2], argc > 3 ? atoi(argv[3]) : 0, "mol", true);
    return 0;
  }
  if (argc >= 3 && !strcmp(argv[1], "--energies")) {
    // testFragorb --energies file.xyz charge : eV, one per line, with occupancy
    Mol m = readXyz(argv[2], argc > 3 ? atoi(argv[3]) : 0);
    FragmentOrbitals::FoSet set; string why;
    if (!wholeMolecule(m, set, why)) { fprintf(stderr, "%s\n", why.c_str()); return 1; }
    for (size_t k = 0; k < set.fos.size(); k++)
      printf("%d %.4f %.1f\n", (int)k + 1, set.fos[k].energy*27.211386245988,
             set.fos[k].occupancy);
    return 0;
  }

  const string dir = argc > 1 ? argv[1] : "";
  if (dir.empty()) { fprintf(stderr, "usage: testFragorb REFDIR\n"); return 2; }
  setenv("FRAGORB_REF", dir.c_str(), 1);

  // 1. H2 at 1.4 bohr, zeta 1.3: S = exp(-x)(1 + x + x^2/3), x = zeta R
  {
    printf("STO overlap through the fit\n");
    Mol m; m.charge = 0; m.sym = { "H", "H" };
    const double R = 1.4*0.529177210903;
    m.xyz = { 0, 0, 0, 0, 0, R };
    FragmentOrbitals::FoSet set; string why;
    bool ok = wholeMolecule(m, set, why);
    check(ok, "H2 builds");
    if (ok) {
      const double x = 1.3*1.4;
      const double exact = exp(-x)*(1 + x + x*x/3);
      const double got = set.overlap[1];
      check(fabs(got - exact) < 2e-4, "H2 1s-1s overlap against the closed form", got - exact);
      check(set.worstFit > 0.999, "STO-6G fit quality", set.worstFit);
    }
  }

  // 2. Cr 3d double zeta normalised and 3d-3d on-site overlaps orthonormal
  {
    printf("Cr atom\n");
    Mol m; m.charge = 0; m.sym = { "Cr" }; m.xyz = { 0, 0, 0 };
    FragmentOrbitals::FoSet set; string why;
    FragmentOrbitals::Fragment f; f.name = "Cr"; f.kind = FragmentOrbitals::METAL;
    f.atoms.push_back(0);
    vector<FragmentOrbitals::Fragment> v(1, f);
    bool ok = FragmentOrbitals::build(m.sym, m.xyz, v, FragmentOrbitals::PER_LIGAND, set, why);
    check(ok, "Cr builds");
    if (ok) {
      check(set.funcs.size() == 9, "nine valence functions", set.funcs.size());
      const size_t n = set.funcs.size();
      double worst = 0;
      for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++) {
          const double want = (i == j) ? 1.0 : 0.0;
          // 4s-4p same-centre are orthogonal by symmetry; 3d vs 4s are not
          if (set.funcs[i].l != set.funcs[j].l && i != j) continue;
          worst = max(worst, fabs(set.overlap[i*n+j] - want));
        }
      check(worst < 1e-6, "on-site same-l overlap is the unit matrix", worst);
    }
  }
  printf("EHT against YAeHMOP bind\n");
  againstYaehmop(dir, "co", 0);
  againstYaehmop(dir, "crco6", 0);
  againstYaehmop(dir, "moco6", 0);   // 4d, 5d and a late 3d metal: the source
  againstYaehmop(dir, "wco6", 0);    // file leaves their double-zeta d
  againstYaehmop(dir, "nico6", 0);   // coefficients unnormalised
  againstYaehmop(dir, "tif6", -2);
  againstYaehmop(dir, "conh36", 3);

  fragments();

  printf("projection on a spanning FO set\n");
  composeSelf((dir + "/crco6.xyz").c_str(), 0, "Cr(CO)6", false);
  composeSelf((dir + "/conh36.xyz").c_str(), 3, "Co(NH3)6", false);
  formulas(dir);
  pairCounts();
  printf(fails ? "FAILED\n" : "ok\n");
  return fails ? 1 : 0;
}
