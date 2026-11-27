// Pocket detection (#1320).  Usage: pocketfinder_test [1iep.pdb]
//  - synthetic: a hollow shell of atoms must give one pocket at its centre; a
//    lone small molecule must give none (an enclosure measure has an exact oracle)
//  - 1iep (ABL kinase + imatinib): the ligand site is recovered once the
//    ligand is ignored.  Exit 77 when the fixture is absent.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include "dock/PocketFinder.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

using namespace std;
using namespace PocketFinder;

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; \
  fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static double dist(const array<double,3>& a, const double *b)
{ return sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2])); }

static void synthetic()
{
  // Carbon atoms on a sphere of radius 8 A, about 1.8 A apart: closed cavity.
  Fragment shell;
  const double R = 8.0;
  for (int i = -10; i <= 10; i++) for (int j = -10; j <= 10; j++) for (int k = -10; k <= 10; k++) {
    double x = i*1.5, y = j*1.5, z = k*1.5, r = sqrt(x*x + y*y + z*z);
    if (r < R - 0.75 || r > R + 0.75) continue;
    double c[3] = { x, y, z };
    shell.addAtom("C", c);
  }
  Result res = findPockets(shell);
  CHECK(res.pockets.size() == 1, "shell: %d pockets, expected 1", (int)res.pockets.size());
  if (!res.pockets.empty()) {
    const double o[3] = { 0, 0, 0 };
    CHECK(dist(res.pockets[0].centre, o) < 1.0, "shell: centre %.2f from origin",
          dist(res.pockets[0].centre, o));
    CHECK(res.pockets[0].volume > 300, "shell: volume %.0f", res.pockets[0].volume);
    CHECK(res.pockets[0].hi[0] < R, "shell: pocket extends outside the shell");
  }
  CHECK(res.radiusFromCovalent == 0 && res.radiusFromDefault == 0, "carbon needs no fallback");

  // Ethane-sized molecule: nothing encloses anything.
  Fragment small;
  const double a[3] = { 0, 0, 0 }, b[3] = { 1.5, 0, 0 }, c[3] = { 2.3, 1.2, 0 };
  small.addAtom("C", a); small.addAtom("C", b); small.addAtom("O", c);
  CHECK(findPockets(small).pockets.empty(), "small molecule gave a pocket");

  // An element with no vdW entry must still occupy space and be reported.
  Fragment odd;
  const double o0[3] = { 0, 0, 0 };
  odd.addAtom("X", o0);
  Result r2 = findPockets(odd);
  CHECK(r2.radiusFromCovalent + r2.radiusFromDefault == 1 && !r2.fallbackElements.empty(),
        "dummy atom: fallback not counted (%d,%d)", r2.radiusFromCovalent, r2.radiusFromDefault);
}

static int real(const string& path)
{
  ifstream in(path);
  if (!in) { printf("SKIP: %s not found\n", path.c_str()); return 77; }
  Fragment f;
  if (!f.restorePDB(in, 1.0, false, 1, " ", true, false, " ", false)) {
    fprintf(stderr, "FAIL: cannot read %s\n", path.c_str()); return 1;
  }
  // Ligand heavy atoms: imatinib (STI) in chain A.  Residue names come padded to 4.
  vector<const double*> lig;
  double cen[3] = { 0, 0, 0 };
  for (size_t i = 0; i < f.numAtoms(); i++) {
    TAtm *t = f.atomRef((int)i);
    if (t->getResidue() && t->getResidue()->name().compare(0, 3, "STI") == 0 &&
        t->getResidue()->chain() == 'A' && t->atomicNumber() > 1) {
      lig.push_back(t->coordinates());
      for (int k = 0; k < 3; k++) cen[k] += t->coordinates()[k];
    }
  }
  CHECK(lig.size() > 25, "STI atoms found: %d", (int)lig.size());
  for (int k = 0; k < 3; k++) cen[k] /= lig.size();

  Result res = findPockets(f);
  printf("1iep: %d pockets, ignored %d ligand atoms, %d water\n", (int)res.pockets.size(),
         res.atomsIgnoredLigand, res.atomsIgnoredWater);
  for (size_t i = 0; i < res.pockets.size() && i < 3; i++) {
    const Pocket& p = res.pockets[i];
    printf("  #%d score %.1f volume %.0f centre-to-ligand %.1f A, %d lining residues\n",
           (int)i + 1, p.score, p.volume, dist(p.centre, cen), (int)p.lining.size());
  }
  CHECK(res.atomsIgnoredLigand >= (int)lig.size(), "ligand not ignored (%d)", res.atomsIgnoredLigand);
  CHECK(!res.pockets.empty(), "no pockets in 1iep");
  if (res.pockets.empty()) return 1;
  const Pocket& top = res.pockets[0];
  // Fraction of ligand heavy atoms with a top-pocket point within 2 A.
  int covered = 0;
  for (const double *q : lig)
    for (const auto& pt : top.pointCoords)
      if (dist(pt, q) < 2.0) { covered++; break; }
  printf("  top pocket covers %d of %d ligand atoms within 2 A\n", covered, (int)lig.size());
  CHECK(covered >= (int)lig.size() / 2, "top pocket covers only %d of %d ligand atoms",
        covered, (int)lig.size());
  CHECK(dist(top.centre, cen) < 8.0, "top pocket centre %.1f A from ligand", dist(top.centre, cen));
  CHECK(top.lining.size() >= 8, "lining residues: %d", (int)top.lining.size());
  return 0;
}

int main(int argc, char **argv)
{
  synthetic();
  int rc = real(argc > 1 ? argv[1] : "tests/dock/fixtures/1iep.pdb");
  if (failures) return 1;
  return rc;
}
