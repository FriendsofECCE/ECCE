#include "tdat/ForceComparison.H"
#include <cmath>
#include <iostream>
#include <limits>

static int failures = 0;
static void check(bool ok, const char *message)
{
  std::cout << (ok ? "PASS: " : "FAIL: ") << message << '\n';
  if (!ok) ++failures;
}
static bool near(double a, double b) { return std::abs(a-b) < 1e-9; }

int main()
{
  ForceInput a;
  a.elements = {"O", "H", "H"};
  a.coordinates = {0,0,0, 1,0,0, 0,1,0};
  a.gradient = {1,2,3, 1,2,3, 1,2,3};
  a.units = "eV/Angstrom";
  a.energy.code = "NWChem";
  a.energy.state.method = "DFT/PBE";
  a.energy.state.energy = -10;
  ForceInput b = a;
  b.gradient = {1,5,7, 1,2,5, 1,2,7}; // vector differences: 5, 2, 4
  b.energy.state.energy += 0.01;
  ForceComparison r = compareForces(a,b);
  check(r.ok && r.rows.size() == 3, "two elements and all atoms");
  check(r.rows[0].element == "H" && r.rows[0].count == 2 &&
        near(r.rows[0].maximum,4) && near(r.rows[0].rms,std::sqrt(10.)), "H max=4, rms=sqrt(10)");
  check(r.rows[1].element == "O" && r.rows[1].count == 1 &&
        near(r.rows[1].maximum,5) && near(r.rows[1].rms,5), "O max=rms=5");
  check(r.rows[2].count == 3 && near(r.rows[2].maximum,5) &&
        near(r.rows[2].rms,std::sqrt(15.)), "all atoms max=5, rms=sqrt(15)");
  check(r.hasEnergy && near(r.deltaEV,0.27211386245988) &&
        near(r.deltaMeVPerAtom,90.70462081996), "B-A energy in eV/cell and meV/atom");
  check(r.notes.empty(), "identical geometry needs no warning");
  b.coordinates[0] += 0.03;
  r = compareForces(a,b);
  check(r.ok && near(r.geometryRms,0.03/std::sqrt(3.)) && !r.notes.empty(), "geometry RMS is per atom and warns above 0.01 A");
  b.energy.xc = "different functional";
  r = compareForces(a,b);
  check(r.ok && !r.hasEnergy && forceComparisonText(r).find(
        "No energy difference: the two calculations use different methods.") != std::string::npos,
        "different functional suppresses only the energy");
  b = a;
  b.energy.code = "CP2K";
  check(!compareForces(a,b).hasEnergy, "different code suppresses energy");
  b = a;
  b.energy.state.method = "other";
  check(!compareForces(a,b).hasEnergy, "different theory suppresses energy");
  b = a;
  b.energy.state.problem = "missing energy";
  check(compareForces(a,b).ok && !compareForces(a,b).hasEnergy, "missing energy preserves forces");
  b = a;
  b.elements = {"H", "O", "H"};
  check(!compareForces(a,b).ok, "same counts but different element sequence refused");
  b.elements.pop_back();
  check(!compareForces(a,b).ok, "different atom counts refused");
  b = a;
  b.gradient.pop_back();
  check(!compareForces(a,b).ok, "short gradient refused");
  b = a;
  b.units = "unknown";
  check(!compareForces(a,b).ok, "unknown gradient units refused");
  b = a;
  b.gradient[0] = std::numeric_limits<double>::quiet_NaN();
  check(!compareForces(a,b).ok, "non-finite data refused");
  a.gradient.assign(9,0);
  b = a;
  a.units = b.units = "Hartree/Bohr";
  b.gradient[0] = 1;
  check(near(compareForces(a,b).rows[1].maximum,51.42208619), "Eh/bohr converted explicitly to eV/A");
  b.units = "eV/Angstrom";
  b.gradient[0] = 51.42208619;
  check(near(compareForces(a,b).rows[1].maximum,51.42208619), "mixed convertible units");
  return failures ? 1 : 0;
}
