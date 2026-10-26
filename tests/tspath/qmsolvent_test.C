#include "tdat/QmSolventWarning.H"
#include <cstdio>

static int fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); fails++; } } while (0)

int main()
{
  CHECK(qmSolventWarning("Water", "Energy").empty(), "energy with solvent is fine");
  CHECK(qmSolventWarning("None", "Geometry", true).empty(), "no solvent is fine");
  CHECK(qmSolventWarning("", "Vibration", true).empty(), "unset solvent is fine");
  for (const char *rt : {"Gradient", "Geometry", "Vibration", "GeoVib",
                         "Transition State (Opt-TS)", "IRC"})
    CHECK(qmSolventWarning("Water", rt).empty(), "every run type works with a solvent");
  std::string w = qmSolventWarning("Water", "Vibration", true);
  CHECK(!w.empty(), "raman with a solvent warns");
  CHECK(w.find("Vibration") != std::string::npos, "names the run type");
  CHECK(w.find("Water") != std::string::npos, "names the solvent");
  printf(fails ? "%d failures\n" : "ok\n", fails);
  return fails != 0;
}
