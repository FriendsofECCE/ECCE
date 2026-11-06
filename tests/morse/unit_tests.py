#!/usr/bin/env python3
"""Unit tests for scripts/ecce_morse.py (#858).

Synthetic Morse curves must be recovered exactly; omega_e is checked against
an independent route (the curvature of the synthetic curve by finite
differences, omega = sqrt(V''/mu)), and the CLI is run on a stored
calculation made here (Props vectors + chemsys.mvm), with and without --json.
The fit of the shipped H2 curve (General Chemistry h2_bond_curve, or --ccsd
<calc>) is printed next to the experimental constants of H2; the differences
are stated, not tested.
"""
import argparse
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import ecce_morse as M  # noqa: E402

fails = []
MU_H2 = 1.007825 / 2
# Huber and Herzberg, H2 X 1Sigma_g+
EXPT = {"re_A": 0.7414, "omega_e_cm-1": 4401.2, "omega_e_xe_cm-1": 121.3, "D0_eV": 4.478}


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else "  " + str(info)))
    if not cond:
        fails.append(name)


def close(a, b, rel):
    return abs(a - b) <= rel * max(abs(b), 1e-300)


def synthetic():
    grid = [round(4.0 - 0.1 * i, 10) for i in range(38)]
    for De_eV, a, re_, Einf in ((4.7, 1.94, 0.7414, -1.0), (0.5, 1.2, 2.1, -460.3),
                               (9.8, 2.6, 1.13, -112.7)):
        De = De_eV / M.HARTREE_EV
        rs = grid if re_ < 1.5 else [1.2 + 0.08 * i for i in range(40)]
        e = [M.morse(x, De, a, re_, Einf) for x in rs]
        res = M.analyse(rs, e, MU_H2)
        tag = "De=%g a=%g re=%g" % (De_eV, a, re_)
        check("recovers De (%s)" % tag, close(res["De_eV"], De_eV, 1e-7), res["De_eV"])
        check("recovers a (%s)" % tag, close(res["a_per_A"], a, 1e-7), res["a_per_A"])
        check("recovers re (%s)" % tag, close(res["re_A"], re_, 1e-8), res["re_A"])
        check("recovers Einf (%s)" % tag, close(res["Einf_hartree"], Einf, 1e-10),
              res["Einf_hartree"])
        # Oracle: harmonic wavenumber from the finite-difference curvature.
        h = 1e-4
        d2 = (M.morse(re_ + h, De, a, re_, 0) - 2 * M.morse(re_, De, a, re_, 0)
              + M.morse(re_ - h, De, a, re_, 0)) / h ** 2      # Eh / A^2
        k = d2 * 4.3597447222071e-18 * 1e20
        w = math.sqrt(k / (MU_H2 * 1.66053906660e-27)) / (2 * math.pi * 2.99792458e10)
        check("omega_e equals sqrt(V''/mu) (%s)" % tag, close(res["omega_e_cm-1"], w, 1e-5),
              (res["omega_e_cm-1"], w))
        check("D0 = De - we/2 + wexe/4 (%s)" % tag,
              close(res["D0_cm-1"], De * M.HARTREE_CM - res["omega_e_cm-1"] / 2
                    + res["omega_e_xe_cm-1"] / 4, 1e-12))
        check("fitted curve passes through the points (%s)" % tag,
              res["rms_kJ_per_mol"] < 1e-6, res["rms_kJ_per_mol"])

    # A window drops the points above it; the fit of exact data is unchanged.
    De = 4.7 / M.HARTREE_EV
    e = [M.morse(x, De, 1.94, 0.7414, -1.0) for x in grid]
    w = M.analyse(grid, e, MU_H2, window_kj=150.0)
    check("window keeps only points within 150 kJ/mol",
          w["points_used"] < 38 and w["window_mode"] == "window"
          and all(M.morse(x, De, 1.94, 0.7414, -1.0) - min(e) <= 150 / M.HARTREE_KJ + 1e-12
                  for x in w["r_used_A"]), w["points_used"])
    check("window fit still exact", close(w["re_A"], 0.7414, 1e-8) and
          close(w["De_eV"], 4.7, 1e-6), (w["re_A"], w["De_eV"]))
    b = M.analyse(grid, e, MU_H2)
    check("default uses the bound points (below the longest-distance energy)",
          b["window_mode"] == "bound" and b["points_used"] == 37, b["points_used"])
    try:
        M.analyse(grid, e, MU_H2, window_kj=0.01)
        check("too narrow a window is refused", False)
    except M.MorseError:
        check("too narrow a window is refused", True)


def formatting():
    check("num: fixed for ordinary values", M.num(4401.23) == "4401" and M.num(0.7414) == "0.7414")
    check("num: no e-notation", M.num(1.234e-3, 2) == "1.2 × 10⁻³", M.num(1.234e-3, 2))
    check("num: large values", M.num(123456.0) == "1.235 × 10⁵", M.num(123456.0))
    rep = M.report(M.analyse([4.0 - 0.1 * i for i in range(38)],
                             [M.morse(4.0 - 0.1 * i, 0.17, 1.9, 0.74, -1.0) for i in range(38)],
                             MU_H2))
    check("report uses real superscripts", "Å⁻¹" in rep and "cm⁻¹" in rep
          and "e-" not in rep and "E-" not in rep, rep)


def stored_calc(work, r, e, elements=("H", "H")):
    calc = os.path.join(work, "h2_test_scan")
    os.makedirs(os.path.join(calc, "Props"))
    os.makedirs(os.path.join(calc, "Parameters"))
    for key, vals, unit in (("SCANCOORD", r, "Angstrom"), ("SCANENERGY", e, "Hartree")):
        open(os.path.join(calc, "Props", key), "w").write(
            '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Scan Point" rows="%d" units="%s">%s</vector>\n'
            % (key, len(vals), unit, "   ".join("%.12f" % v for v in vals)))
    atoms = "\n".join("%s 0.0 0.0 %.3f" % (el, 0.74 * i) for i, el in enumerate(elements))
    open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w").write(
        "title: h2\ntype: molecule\nnum_atoms: %d\natom_info: symbol cart\natom_list: \n%s\n"
        "attr_list:\n" % (len(elements), atoms))
    return calc


def cli():
    work = tempfile.mkdtemp(prefix="ecce-morse-")
    try:
        grid = [4.0 - 0.1 * i for i in range(38)]
        e = [M.morse(x, 4.7 / M.HARTREE_EV, 1.94, 0.7414, -1.0) for x in grid]
        calc = stored_calc(work, grid, e)
        exe = os.path.join(ROOT, "scripts", "ecce_morse.py")
        p = subprocess.run([sys.executable, exe, calc, "--json"], capture_output=True, text=True)
        check("CLI --json runs", p.returncode == 0, p.stderr)
        j = json.loads(p.stdout) if p.returncode == 0 else {}
        check("CLI finds H-H and the reduced mass from Isotopes.txt",
              j.get("bond") == "H1–H2" and close(j.get("mu_amu", 0), MU_H2, 1e-6), j.get("bond"))
        check("CLI recovers re", close(j.get("re_A", 0), 0.7414, 1e-8))
        check("CLI JSON has the fitted curve", len(j.get("curve_r_A", [])) == M.CURVE_POINTS)
        p = subprocess.run([sys.executable, exe, "file://" + calc], capture_output=True, text=True)
        check("CLI text report", p.returncode == 0 and p.stdout.startswith("Morse fit of h2_test_scan"),
              p.stdout + p.stderr)
        p = subprocess.run([sys.executable, exe, calc, "--window", "100", "--json"],
                           capture_output=True, text=True)
        j2 = json.loads(p.stdout) if p.returncode == 0 else {}
        check("CLI --window", j2.get("window_mode") == "window"
              and j2.get("points_used", 99) < j.get("points_used", 0), p.stderr)
        calc3 = stored_calc(os.path.join(work, "x"), grid, e, ("O", "H", "H"))
        p = subprocess.run([sys.executable, exe, calc3, "--json"], capture_output=True, text=True)
        check("CLI refuses an unknown bond in a polyatomic", p.returncode == 1 and
              "--atoms" in json.loads(p.stdout).get("error", ""), p.stdout)
        p = subprocess.run([sys.executable, exe, calc3, "--atoms", "1,2", "--json"],
                           capture_output=True, text=True)
        check("CLI --atoms names the bond", p.returncode == 0 and
              json.loads(p.stdout).get("bond") == "O1–H2", p.stdout + p.stderr)
    finally:
        shutil.rmtree(work, True)


def ccsd(calc):
    res = M.run(calc)
    print(res["report"])
    print("\nCompared with experiment for H2 (Huber-Herzberg); differences = fit - experiment:")
    for k, unit in (("re_A", "A"), ("omega_e_cm-1", "cm-1"), ("omega_e_xe_cm-1", "cm-1"),
                    ("D0_eV", "eV")):
        print("  %-16s fit %10.4f  expt %10.4f  diff %+9.4f %s" % (k, res[k], EXPT[k],
                                                                res[k] - EXPT[k], unit))
    check("CCSD H2 scan fits with re near 0.74 A", 0.70 < res["re_A"] < 0.78, res["re_A"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ccsd", default=os.path.join(
        ROOT, "data", "client", "examples", "genchem", "bond-formation",
        "orca-h2_bond_curve", "stored", "h2_bond_curve"),
        help="stored H2 CCSD scan calculation")
    a = ap.parse_args()
    synthetic()
    formatting()
    cli()
    if os.path.isdir(a.ccsd):
        ccsd(a.ccsd)
    print("FAILED: %d" % len(fails) if fails else "PASSED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
