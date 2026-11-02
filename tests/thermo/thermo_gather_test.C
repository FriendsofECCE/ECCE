// gatherThermoInput on four real stored geometries (#692): the total mass,
// the rotational constants, the linear flag and the symmetry number are all
// derived from the stored molecule and point group, never supplied by the
// test.  Water (C2v), ammonia (C3v), CO2 (linear Dinfh) and methane (Td).
// The geometries are ECCE's structure-library coordinates (the same ones
// tests/modiagram/makecalc.py stores); each is a minimal FileEDSI
// calculation under fixtures/, with only TE and PNTGRP as properties.
//
// The expected masses are the standard-atomic-weight molecular weights and
// the expected rotational constants are hand-computed from those geometries
// (CODATA constants, ordinary principal-axis formulas), not read back from
// the code under test.  TE is a placeholder: it is not asserted here.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ThermoGather.H"

using std::string;

static int failures = 0;
static void check(bool ok, const string& what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

static bool near(double a, double b, double tol) { return std::fabs(a - b) < tol; }

struct Case {
  const char* dir;
  const char* label;
  double mass;       // amu
  double rot[3];     // cm-1, ascending-inertia order (rotConstants[0..2])
  bool linear;
  int sigma;
};

static const Case CASES[] = {
  { "h2o", "water C2v",  18.015280, {27.373675, 14.585140, 9.515257}, false, 2 },
  { "nh3", "ammonia C3v", 17.030520, {9.902057, 9.902057, 6.340343}, false, 3 },
  { "co2", "CO2 Dinfh",  44.009500, {0.391514, 0.391514, 0.391514}, true,  2 },
  { "ch4", "methane Td", 16.042460, {5.308038, 5.308038, 5.308038}, false, 12 },
};

int main(int argc, char** argv)
{
  const string src = argc > 1 ? argv[1] : ".";
  const string base = src + "/tests/thermo/fixtures/";

  for (const Case& c : CASES) {
    ThermoInput in;
    string name, error;
    std::vector<string> notes;
    const bool ok = gatherThermoInput("file://" + base + c.dir, in, name,
                                      notes, error);
    printf("%s: mass %.6f amu, B %.6f %.6f %.6f cm-1, linear %d, sigma %d\n",
           c.label, in.mass, in.rotConstants[0], in.rotConstants[1],
           in.rotConstants[2], in.linear ? 1 : 0, in.symmetryNumber);
    check(ok && error.empty(), c.label + string(": gathered (") + error + ")");
    check(near(in.mass, c.mass, 1e-3), c.label + ": total mass to 1e-3 amu");
    for (int i = 0; i < 3; i++) {
      char buf[128];
      snprintf(buf, sizeof buf, "%s: B[%d] within 1%%", c.label, i);
      check(near(in.rotConstants[i], c.rot[i], 0.01 * c.rot[i]), buf);
    }
    check(in.linear == c.linear, c.label + string(": linear detection"));
    check(in.symmetryNumber == c.sigma, c.label + string(": symmetry number"));
  }

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
