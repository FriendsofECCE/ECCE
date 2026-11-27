// van der Waals radii from TPerTab::vwr (#1320): present and sane for the
// elements a protein contains plus common metals.  Usage: vwr_radii_test
#include <cstdio>
#include <string>

#include "tdat/TPerTab.H"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main() {
  TPerTab tpt;
  // Angstrom; bounds bracket the usual published vdW radii for each element.
  struct { const char* sym; double lo, hi; } want[] = {
    {"H", 1.0, 1.3}, {"C", 1.4, 1.8}, {"N", 1.3, 1.7}, {"O", 1.3, 1.6},
    {"S", 1.7, 2.0}, {"P", 1.5, 2.0}, {"Mg", 1.4, 2.0}, {"Ca", 1.8, 2.5},
    {"Fe", 1.3, 2.3}, {"Zn", 1.2, 1.8}, {"Cu", 1.2, 1.8}, {"Mn", 1.3, 2.3},
  };
  for (const auto& w : want) {
    int z = tpt.atomicNumber(w.sym);
    CHECK(z > 0, "no atomic number for %s", w.sym);
    double r = tpt.vwr(z);
    CHECK(r >= w.lo && r <= w.hi, "%s vwr %.3f outside [%.2f, %.2f]",
          w.sym, r, w.lo, w.hi);
  }
  // No real element (Z <= 118; Nub and X are placeholders) may be zero or absurd.
  for (int z = 1; z <= 118 && z < tpt.numAtoms(); ++z) {
    double r = tpt.vwr(z);
    CHECK(r > 0.5 && r < 4.0, "Z=%d (%s) vwr %.3f", z,
          tpt.atomicSymbol(z).c_str(), r);
  }
  if (failures) return 1;
  printf("vwr_radii_test: OK\n");
  return 0;
}
