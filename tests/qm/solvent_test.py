#!/usr/bin/env python3
"""CPCM solvation energies E(solvent) - E(vacuum) of ecce-qm against ORCA 6.1.1.

Oracle: ORCA `! HF 6-31G* TightSCF CPCM(water)` (epsilon 80.151, Bondi radii
x 1.2, H 1.10) minus the vacuum run, kJ/mol, 2026-10-25 (geometries in
MOLS).  usage: solvent_test.py <ecce-qm>
"""
import os, re, subprocess, sys, tempfile

H_KJ = 2625.4996394799
MOLS = {  # name: (charge, atoms, ORCA kJ/mol, tolerance kJ/mol)
    "h2o": (0, [("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692), ("H", 0, -0.7572, -0.4692)], -30.29, 0.5),
    "nh3": (0, [("N", 0, 0, 0.1161), ("H", 0, 0.9377, -0.2709), ("H", 0.8121, -0.4689, -0.2709),
                ("H", -0.8121, -0.4689, -0.2709)], -24.24, 0.5),
    "cl-": (-1, [("Cl", 0, 0, 0)], -315.35, 1.5),
}


def energy(exe, charge, atoms, extra):
    deck = "charge %d\nmultiplicity 1\nmethod hf\nbasis 6-31G*\n%sgeometry\n%send\n" % (
        charge, extra, "".join("%s %s %s %s\n" % a for a in atoms))
    with tempfile.NamedTemporaryFile("w", suffix=".qm", delete=False) as f:
        f.write(deck)
    try:
        out = subprocess.run([exe, f.name], capture_output=True, text=True, check=True).stdout
    finally:
        os.unlink(f.name)
    return float(re.search(r"^energy_total (\S+)", out, re.M).group(1))


def main():
    exe = sys.argv[1]
    bad = 0
    for name, (q, atoms, ref, tol) in MOLS.items():
        d = (energy(exe, q, atoms, "epsilon 80.151\n") - energy(exe, q, atoms, "")) * H_KJ
        ok = abs(d - ref) <= tol
        bad += not ok
        print("%-5s %s  ecce-qm %8.2f  ORCA %8.2f  kJ/mol" % (name, "ok  " if ok else "FAIL", d, ref))
    # an unsupported run type must be refused, never silently run without solvent
    deck = "charge 0\nmultiplicity 1\nmethod hf\nbasis STO-3G\nsolvent water\nruntype optimize\ngeometry\nH 0 0 0\nH 0 0 0.74\nend\n"
    with tempfile.NamedTemporaryFile("w", suffix=".qm", delete=False) as f:
        f.write(deck)
    try:
        r = subprocess.run([exe, f.name], capture_output=True, text=True)
    finally:
        os.unlink(f.name)
    refused = r.returncode != 0 and "solvent" in r.stderr and "runtype energy" in r.stderr
    print("optimize with solvent refused:", "ok" if refused else "FAIL")
    bad += not refused
    return 1 if bad else 0


sys.exit(main())
