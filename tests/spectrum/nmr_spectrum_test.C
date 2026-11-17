#include "tdat/NmrSpectrum.H"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void check(bool ok, const char *name) {
  std::cout << (ok ? "PASS " : "FAIL ") << name << '\n';
  if (!ok) std::exit(1);
}
int main() {
  NmrSpectrum data;
  data.set({"C", "H", "H", "H", "H", "F"}, {180, 31, 30.995, 30.989, 32, 200});
  check(data.elements() == std::vector<std::string>({"H", "C", "F"}), "nucleus ordering");
  check(!data.meanShielding("N"), "missing reference element");
  check(std::fabs(*data.meanShielding("H") - 31.246) < 1e-10, "reference element mean");
  check(data.spectrum("H", std::nullopt).sticks(VIB_IR).empty(), "empty reference gives no shifts");
  auto gs = data.groups("H", 31);
  check(gs.size() == 3 && gs[0].shift == -1 && gs[1].atoms == std::vector<int>({1, 2})
        && gs[2].atoms == std::vector<int>({3}), "signed shifts, bounded grouping and atom indices");
  check(std::fabs(gs[1].shift - 0.0025) < 1e-10, "group mean shift");
  auto spec = data.spectrum("H", 31);
  spec.setFwhm(0.05);
  check(spec.sticks(VIB_IR).size() == 3 && spec.sticks(VIB_IR)[1].intensity == 2,
        "zero and negative shifts retained with nucleus counts");
  check(spec.xTitle() == "Chemical shift (ppm)" && spec.itemWord() == "Group"
        && spec.axisLabel(VIB_IR) == "Nucleus count", "ppm wording");
  check(std::fabs(VibSpectrum::lineValue(LINE_LORENTZIAN, 0.025, 0.05) /
                  VibSpectrum::lineValue(LINE_LORENTZIAN, 0, 0.05) - 0.5) < 1e-12,
        "Lorentzian FWHM");
  double integral = 0, step = 0.002;
  for (double x = -100; x < 100; x += step) integral += spec.envelope(VIB_IR, x) * step;
  check(std::fabs(integral - 4) < 0.002, "envelope area equals nucleus count");
  double lo, hi;
  spec.defaultRange(&lo, &hi);
  check(lo < -1 && hi > 0.011, "ppm range contains negative and near-zero shifts");
  check(data.spectrum("C", 190).sticks(VIB_IR)[0].wavenumber == 10, "carbon shift");
  check(data.spectrum("F", 180).sticks(VIB_IR)[0].wavenumber == -20, "other element with reference");
  auto csv = data.csv("H", 31, "reference, test", 0.05);
  check(csv.find("atoms_1based") != std::string::npos && csv.find("\"2 3\"") != std::string::npos
        && csv.find("shielding_ppm") != std::string::npos
        && csv.find("envelope_nuclei_per_ppm") != std::string::npos, "CSV provenance, atoms and curve");
  bool rejected = false;
  try { data.set({"H"}, {}); } catch (const std::exception&) { rejected = true; }
  check(rejected, "mismatched atom data rejected");
  rejected = false;
  try { data.groups("H", std::numeric_limits<double>::quiet_NaN()); }
  catch (const std::exception&) { rejected = true; }
  check(rejected, "nonfinite reference rejected");
  return 0;
}
