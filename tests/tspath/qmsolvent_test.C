#include "tdat/QmSolventWarning.H"
#include <cstdio>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails++; } } while (0)

int main()
{
  CHECK(qmSolventWarning("Water", "Energy").empty(), "energy with solvent is fine");
  CHECK(qmSolventWarning("None", "Geometry").empty(), "no solvent is fine");
  CHECK(qmSolventWarning("", "Vibration").empty(), "unset solvent is fine");
  for (const char *rt : {"Gradient", "Geometry", "Vibration", "GeoVib",
                         "Transition State (Opt-TS)", "IRC"}) {
    std::string w = qmSolventWarning("Water", rt);
    CHECK(!w.empty(), rt);
    CHECK(w.find(rt) != std::string::npos, "names the run type");
    CHECK(w.find("Water") != std::string::npos, "names the solvent");
  }
  CHECK(!qmSolventWarning("Custom", "Geometry").empty(), "custom epsilon too");
  printf(fails ? "%d failures\n" : "ok\n", fails);
  return fails != 0;
}
