// Recomputing H, S, G from stored frequencies (#692).  The oracles are
// ORCA's own printed thermochemistry (Grimme qRRHO, w0 = 100 cm-1) for two
// stored outputs in the repo, and a hand computation of plain RRHO for H2O.
//
// ORCA fixtures used (numbers below are read straight from them):
//   tests/parsers/fixtures/orca/h2o_optfreq.out
//   tests/modecomp/nma_trans_freq.out  (has a 43.28 cm-1 mode, exercises qRRHO)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/thermo/ThermoChem.H"

using std::string;
using std::vector;

static int failures = 0;
static void check(bool ok, const char* what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}
static bool near(double a, double b, double tol = 1e-9)
{
  return std::fabs(a - b) < tol;
}

// 0.05 kJ/mol (the stated oracle tolerance) in hartree.
static const double KJ = 4.3597447222071e-18 * 6.02214076e23 / 1000.0;
static const double TOL = 0.05 / KJ;

static ThermoInput h2o()
{
  ThermoInput in;
  in.electronicEnergy = -74.96590119;
  in.frequencies = {2169.82, 4139.63, 4390.66};
  in.mass = 18.02;
  in.rotConstants[0] = 23.291818;
  in.rotConstants[1] = 14.550433;
  in.rotConstants[2] = 8.955758;
  in.symmetryNumber = 2;
  in.multiplicity = 1;
  return in;
}

static ThermoInput nma()
{
  ThermoInput in;
  in.electronicEnergy = -248.06214658;
  in.frequencies = {43.28, 133.36, 165.16, 282.32, 487.96, 507.44, 584.54,
                    637.78, 833.32, 983.40, 1039.36, 1099.39, 1139.35, 1220.79,
                    1332.81, 1400.54, 1431.28, 1450.15, 1456.16, 1465.80,
                    1481.00, 1513.92, 1852.18, 3030.12, 3065.39, 3109.14,
                    3152.76, 3169.62, 3209.93, 3656.58};
  in.mass = 73.09;
  in.rotConstants[0] = 0.295048;
  in.rotConstants[1] = 0.137995;
  in.rotConstants[2] = 0.097520;
  in.symmetryNumber = 1;
  in.multiplicity = 1;
  return in;
}

int main()
{
  // Plain RRHO for H2O against the hand computation (textbook formulas,
  // CODATA constants, worked out separately; see the values here).
  {
    const ThermoResult r = computeThermo(h2o(), 298.15, THERMO_GAS_1ATM,
                                         THERMO_RRHO);
    check(near(r.H, -74.9377475260, 1e-7), "H2O RRHO H matches the hand value");
    check(near(r.G, -74.9592632169, 1e-7), "H2O RRHO G matches the hand value");
    check(near(r.S, 189.466506, 1e-6), "H2O RRHO S (J/mol/K) matches");
    check(near(r.sTrans, 144.807546, 1e-6) && near(r.sRot, 44.656255, 1e-6) &&
          near(r.sElec, 0.0, 1e-12), "H2O RRHO entropy pieces match");
    check(near(r.zpe, 0.0243766442, 1e-8), "H2O RRHO ZPE matches");
  }

  // ORCA oracle 1: H2O optfreq, Grimme qRRHO w0 = 100 (ORCA G -74.95926274).
  {
    const ThermoResult r = computeThermo(h2o(), 298.15, THERMO_GAS_1ATM,
                                         THERMO_GRIMME_QRRHO, 100.0);
    check(std::fabs(r.G - (-74.95926274)) < TOL, "ORCA h2o G (qRRHO w0=100)");
    check(std::fabs(r.H - (-74.93774751)) < TOL, "ORCA h2o H");
    check(std::fabs(r.S * 298.15 / (KJ * 1000.0) - 0.02151523) < TOL,
          "ORCA h2o T*S in hartree");
  }

  // ORCA oracle 2: NMA (43.28 cm-1 mode), Grimme qRRHO w0 = 100.
  {
    const ThermoResult r = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                         THERMO_GRIMME_QRRHO, 100.0);
    check(std::fabs(r.G - (-247.98931276)) < TOL, "ORCA nma G (qRRHO w0=100)");
    // Final entropy term T*S = 0.03690932 Eh in ORCA.
    const double ts = r.S * 298.15 / (KJ * 1000.0);
    check(std::fabs(ts - 0.03690932) < TOL, "ORCA nma T*S");
  }

  // The treatments disagree only where there are soft modes.
  {
    const double gRrho = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                       THERMO_RRHO).G;
    const double gGrimme = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                         THERMO_GRIMME_QRRHO, 100.0).G;
    const double gTruhlar = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                          THERMO_TRUHLAR_RAISE).G;
    const double gHg = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                     THERMO_HEADGORDON_QRRHO).G;
    check(gGrimme > gRrho && gTruhlar > gRrho,
          "NMA: qRRHO and raise-to-100 raise G above plain RRHO");
    const double lo = std::min(std::min(gRrho, gGrimme), std::min(gTruhlar, gHg));
    const double hi = std::max(std::max(gRrho, gGrimme), std::max(gTruhlar, gHg));
    check((hi - lo) * KJ > 1.0, "NMA: the treatments spread more than 1 kJ/mol");
    // H2O has no soft mode, so every treatment agrees.
    const double hRrho = computeThermo(h2o(), 298.15, THERMO_GAS_1ATM,
                                       THERMO_RRHO).G;
    const double hGrimme = computeThermo(h2o(), 298.15, THERMO_GAS_1ATM,
                                         THERMO_GRIMME_QRRHO, 100.0).G;
    check(std::fabs(hRrho - hGrimme) < 1e-7, "H2O: qRRHO equals RRHO");
    // Head-Gordon changes the enthalpy of soft modes but not the ZPE.
    const ThermoResult hg = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                          THERMO_HEADGORDON_QRRHO);
    const ThermoResult rr = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                          THERMO_RRHO);
    check(hg.H != rr.H && near(hg.zpe, rr.zpe, 1e-12),
          "Head-Gordon: enthalpy moves, ZPE stays harmonic");
  }

  // 1 M standard state raises G by RT ln(cRT/p) = 7.93 kJ/mol at 298.15 K.
  {
    const double g1atm = computeThermo(h2o(), 298.15, THERMO_GAS_1ATM,
                                       THERMO_RRHO).G;
    const double g1m = computeThermo(h2o(), 298.15, THERMO_SOLN_1M,
                                     THERMO_RRHO).G;
    check(std::fabs((g1m - g1atm) * KJ - 7.93) < 0.01,
          "1 M vs 1 atm standard state");
  }

  // Symmetry numbers from the point group.
  check(symmetryNumberForPointGroup("C2v") == 2 &&
        symmetryNumberForPointGroup("C2V") == 2 &&
        symmetryNumberForPointGroup("C1") == 1 &&
        symmetryNumberForPointGroup("Cs") == 1 &&
        symmetryNumberForPointGroup("D3h") == 6 &&
        symmetryNumberForPointGroup("Td") == 12 &&
        symmetryNumberForPointGroup("Oh") == 24 &&
        symmetryNumberForPointGroup("Dinfh") == 2 &&
        symmetryNumberForPointGroup("Cinfv") == 1 &&
        symmetryNumberForPointGroup("no-such-group") == 1,
        "point group -> symmetry number");

  // Translation/rotation modes dropped, imaginary modes skipped with a note.
  {
    vector<string> notes;
    const vector<double> freqs = realVibrations(
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -123.4, 2169.82, 4139.63, 4390.66},
        notes);
    check(freqs.size() == 3 && freqs[0] == 2169.82 && freqs[2] == 4390.66,
          "trans/rot dropped and imaginary skipped");
    bool hasTrans = false, hasImag = false;
    for (const string& n : notes) {
      if (n.find("translation/rotation") != string::npos) hasTrans = true;
      if (n.find("imaginary") != string::npos) hasImag = true;
    }
    check(hasTrans && hasImag, "notes mention dropped and imaginary modes");
  }

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
