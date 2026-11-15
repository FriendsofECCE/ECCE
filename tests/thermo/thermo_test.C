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

static ThermoInput singleMode(double nu)
{
  ThermoInput in;
  in.electronicEnergy = -10.0;
  in.frequencies = {nu};
  in.mass = 20.0;
  in.rotConstants[0] = in.rotConstants[1] = in.rotConstants[2] = 1.0;
  in.symmetryNumber = 1;
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

  {
    vector<string> notes;
    const vector<double> freqs = realVibrations(
        {-0.0, -0.01, -0.9, -50.0, 1200.0}, notes);
    check(freqs.size() == 1 && freqs[0] == 1200.0,
          "numerical negative zeros do not count as imaginary vibrations");
    check(notes.size() == 2 && notes[0].find("3 translation/rotation") == 0 &&
          notes[1].find("1 imaginary") == 0,
          "three zeros and one imaginary mode are reported");

    notes.clear();
    realVibrations({1200.0, -0.9, -50.0, -0.01, -0.0}, notes);
    check(notes.size() == 2 && notes[0].find("3 translation/rotation") == 0 &&
          notes[1].find("1 imaginary") == 0,
          "zero classification does not depend on frequency ordering");
  }

  // --treatment qrrho-grimme:50 must parse (the CLI used wrong compare
  // lengths, so the w0-bearing treatments never matched).
  {
    ThermoTreatment t;
    double w0 = 0.0;
    string err;
    check(parseTreatmentName("qrrho-grimme:50", t, w0, err) &&
          t == THERMO_GRIMME_QRRHO && w0 == 50.0, "qrrho-grimme:50 -> w0=50");
    check(parseTreatmentName("qrrho:200", t, w0, err) &&
          t == THERMO_GRIMME_QRRHO && w0 == 200.0, "qrrho:200 -> w0=200");
    check(!parseTreatmentName("qrrho-grimme:0", t, w0, err) &&
          err.find("w0") != string::npos, "qrrho-grimme:0 is rejected");
    const double g50 = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                     THERMO_GRIMME_QRRHO, 50.0).G;
    const double g100 = computeThermo(nma(), 298.15, THERMO_GAS_1ATM,
                                      THERMO_GRIMME_QRRHO, 100.0).G;
    check(std::fabs(g50 - g100) > 1e-7,
          "NMA: qRRHO w0=50 gives a different G than w0=100");
  }

  // Truhlar raises a soft mode for ZPE and thermal energy too, not just the
  // entropy: H of a 30 cm-1 mode equals plain RRHO H of the same mode raised
  // to 100 cm-1 (entropy cancels out of H, so the old entropy-only version
  // would have left H unchanged).
  {
    const ThermoResult t = computeThermo(singleMode(30.0), 298.15,
                                         THERMO_GAS_1ATM, THERMO_TRUHLAR_RAISE);
    const ThermoResult rr = computeThermo(singleMode(30.0), 298.15,
                                          THERMO_GAS_1ATM, THERMO_RRHO);
    const ThermoResult hi = computeThermo(singleMode(100.0), 298.15,
                                          THERMO_GAS_1ATM, THERMO_RRHO);
    check(near(t.H, hi.H, 1e-12), "Truhlar H(30 cm-1) equals RRHO H(100 cm-1)");
    check(std::fabs((t.H - rr.H) - 1.6603647969e-05) < 1e-10,
          "Truhlar raises ZPE + thermal energy by the 30->100 cm-1 amount");
  }

  // Head-Gordon quasi-RRHO enthalpy for one 50 cm-1 mode, worked out here
  // by hand: w = 1 / (1 + (w0/nu)^4) with w0 = 100 cm-1 and alpha = 4, and
  // the damped thermal energy is w*hc nu/(exp(hc nu/kT)-1) + (1-w)*kT/2.
  {
    const double nu = 50.0, T = 298.15, w0 = 100.0;
    const double h = 6.62607015e-34, c = 2.99792458e10, k = 1.380649e-23;
    const double EH = 4.3597447222071e-18;
    const double w = 1.0 / (1.0 + std::pow(w0 / nu, 4.0));
    const double x = h * c * nu / (k * T);
    const double expected = w * (h * c * nu / (std::exp(x) - 1.0)) / EH +
                            (1.0 - w) * 0.5 * k * T / EH;
    const ThermoResult r = computeThermo(singleMode(nu), T, THERMO_GAS_1ATM,
                                         THERMO_HEADGORDON_QRRHO);
    check(near(r.thermalVib, expected, 1e-6),
          "Head-Gordon damped enthalpy matches the hand formula");
  }

  // Symmetry number, rotor type and the near-symmetry warning (#854).
  {
    check(symmetryNumberForPointGroup("D*h") == 2, "D*h (autosym spelling) has sigma 2");
    check(symmetryNumberForPointGroup("Td") == 12, "Td has sigma 12");
    check(pointGroupIsLinear("D*h") && pointGroupIsLinear("C*v") &&
          !pointGroupIsLinear("D3h"), "only C*v and D*h are linear");

    // HgI2 case: bent by a hair, so C2v / sigma 2 became C1 / sigma 1.
    ThermoInput hgi2;
    hgi2.code = "ORCA";
    hgi2.pointGroup = "C1";
    hgi2.symmetryNumber = 1;
    hgi2.sigmaSource = "code";
    hgi2.nearGroup = "D*h";
    hgi2.nearTolerance = 0.05;
    hgi2.rotConstants[0] = 0.05; hgi2.rotConstants[1] = 0.0100;
    hgi2.rotConstants[2] = 0.0099;
    const std::string w = symmetryWarning(hgi2);
    check(w.find("D*h") != std::string::npos && w.find("symmetry number 2") != std::string::npos &&
          w.find("linear rotor") != std::string::npos, "near-D*h structure is warned about");
    check(symmetrySummary(hgi2).find("printed by ORCA") != std::string::npos &&
          symmetrySummary(hgi2).find("non-linear rotor") != std::string::npos,
          "the summary names the source and the rotor");

    // HgI4(2-) case: Td would give sigma 12.
    ThermoInput hgi4 = hgi2;
    hgi4.nearGroup = "Td"; hgi4.symmetryNumber = 1;
    check(symmetryWarning(hgi4).find("symmetry number 12") != std::string::npos,
          "near-Td structure is warned about with sigma 12");
    hgi4.symmetryNumber = 12;
    check(symmetryWarning(hgi4).empty(), "no warning once sigma 12 is used");
    hgi4.nearGroup.clear();
    check(symmetryWarning(hgi4).empty(), "no warning without a near group");

    ThermoInput lin = hgi2;
    std::string err;
    check(applyRotorChoice(lin, 1, err) && lin.linear &&
          near(lin.rotConstants[0], 0.00995, 1e-12), "linear rotor from the two small constants");
    check(!applyRotorChoice(lin, 2, err) && !err.empty(),
          "non-linear cannot be forced on a linear molecule");
    lin.symmetryNumber = 2;
    check(symmetryWarning(lin).empty(), "linear and sigma 2: nothing to warn about");
    // sigma 2 against 1 halves the partition function: S drops by R ln 2.
    ThermoInput a = hgi2, b = hgi2;
    a.mass = b.mass = 200.0; a.frequencies = b.frequencies = {100.0, 150.0, 400.0}; a.symmetryNumber = 1; b.symmetryNumber = 2;
    const double dS = computeThermo(a, 298.15, THERMO_GAS_1ATM, THERMO_RRHO).S -
                      computeThermo(b, 298.15, THERMO_GAS_1ATM, THERMO_RRHO).S;
    check(near(dS, 8.314462618 * std::log(2.0), 1e-6), "sigma 2 lowers S by R ln 2");
  }

  // Contributions to U, S and Cv (#1044).  Oracles: equipartition, and the
  // NIST-JANAF standard entropies of Ar (154.846) and N2 (191.609 J/mol/K),
  // which are at 1 bar: at 1 atm S is lower by R ln(1.01325).
  {
    const double R = 8.314462618, T = 298.15;
    ThermoInput ar;
    ar.atom = true;
    ar.mass = 39.948;
    ar.electronicEnergy = -527.0;
    const ThermoResult r = computeThermo(ar, T, THERMO_GAS_1ATM, THERMO_RRHO);
    check(r.ok, "an atom without frequencies is computed");
    check(near(r.thermalTrans * KJ, 1.5 * R * T / 1000.0, 1e-6), "Ar: U_trans = 3/2 RT");
    check(r.thermalRot == 0.0 && r.thermalVib == 0.0 && r.sRot == 0.0, "Ar: no rotation, no vibration");
    check(near(r.Cv, 1.5 * R, 1e-9) && near(r.Cp, 2.5 * R, 1e-9), "Ar: Cv = 3/2 R, Cp = 5/2 R");
    const double bar = R * std::log(1.01325);
    printf("      Ar S(1 atm) = %.4f\n", r.S);
    check(std::fabs(r.S + bar - 154.846) < 0.05, "Ar: S = 154.85 J/mol/K at 1 bar (Sackur-Tetrode)");

    ThermoInput n2;
    n2.mass = 28.014;
    n2.linear = true;
    n2.rotConstants[0] = n2.rotConstants[1] = n2.rotConstants[2] = 1.99824;
    n2.frequencies = {2358.57};
    n2.symmetryNumber = 2;
    const ThermoResult n = computeThermo(n2, T, THERMO_GAS_1ATM, THERMO_RRHO);
    check(near(n.thermalRot * KJ, R * T / 1000.0, 1e-6), "N2: U_rot = RT");
    check(near(n.cvRot, R, 1e-9), "N2: Cv_rot = R");
    check(n.cvVib > 0.0 && n.cvVib < 0.01 * R, "N2 at 298 K: tiny vibrational Cv");
    check(std::fabs(n.Cv - 2.5 * R) < 0.01 * R, "N2 at 298 K: Cv = 5/2 R");
    printf("      N2 S(1 atm) = %.4f\n", n.S);
    check(std::fabs(n.S + bar - 191.609) < 0.1, "N2: S = 191.61 J/mol/K at 1 bar with sigma 2");
    const ThermoResult hot = computeThermo(n2, 3000.0, THERMO_GAS_1ATM, THERMO_RRHO);
    check(hot.cvVib > 0.8 * R, "N2 at 3000 K: the vibration approaches R");

    const ThermoResult w = computeThermo(h2o(), T, THERMO_GAS_1ATM, THERMO_RRHO);
    check(near(w.thermalRot * KJ, 1.5 * R * T / 1000.0, 1e-6) && near(w.cvRot, 1.5 * R, 1e-9),
          "H2O: U_rot = 3/2 RT, Cv_rot = 3/2 R");
    check(near(w.sTrans + w.sRot + w.sVib + w.sElec, w.S, 1e-9), "S contributions add up");
    check(near(w.Cv, w.cvTrans + w.cvRot + w.cvVib + w.cvElec, 1e-12) && near(w.Cp - w.Cv, R, 1e-9),
          "Cv contributions add up, Cp = Cv + R");

    // Cv is dU/dT for every treatment.
    for (ThermoTreatment t : {THERMO_RRHO, THERMO_GRIMME_QRRHO, THERMO_TRUHLAR_RAISE,
                              THERMO_HEADGORDON_QRRHO}) {
      const ThermoInput s = nma();
      const double h = 0.01;
      const ThermoResult up = computeThermo(s, T + h, THERMO_GAS_1ATM, t);
      const ThermoResult dn = computeThermo(s, T - h, THERMO_GAS_1ATM, t);
      const ThermoResult at = computeThermo(s, T, THERMO_GAS_1ATM, t);
      const double dU = ((up.thermalVib - dn.thermalVib) * KJ * 1000.0) / (2 * h);
      check(std::fabs(dU - at.cvVib) < 1e-4, (string("Cv_vib = dU_vib/dT, ") + treatmentName(t)).c_str());
    }

    // ECCE-QM prints sigma 6 for ethane (#1043): S drops by R ln 6.
    ThermoInput e1 = h2o(), e6 = h2o();
    e1.symmetryNumber = 1; e6.symmetryNumber = 6;
    check(near(computeThermo(e1, T, THERMO_GAS_1ATM, THERMO_RRHO).S -
               computeThermo(e6, T, THERMO_GAS_1ATM, THERMO_RRHO).S, R * std::log(6.0), 1e-6),
          "sigma 6 lowers S by R ln 6");
  }

  printf("%d failure(s)\n", failures);
  return failures ? 1 : 0;
}
