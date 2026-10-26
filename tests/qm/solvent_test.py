#!/usr/bin/env python3
"""CPCM solvation energies E(solvent) - E(vacuum) of ecce-qm against ORCA 6.1.1.

Oracle: ORCA `! HF 6-31G* TightSCF CPCM(water)` (epsilon 80.151, Bondi radii
x 1.2, H 1.10) minus the vacuum run, kJ/mol, 2026-10-25 (geometries in
MOLS).  usage: solvent_test.py <ecce-qm>
"""
import math, os, re, subprocess, sys, tempfile

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


def run_deck(exe, deck):
    with tempfile.NamedTemporaryFile("w", suffix=".qm", delete=False) as f:
        f.write(deck)
    try:
        return subprocess.run([exe, f.name], capture_output=True, text=True)
    finally:
        os.unlink(f.name)


# ORCA 6.1.1, `! HF 6-31G* TightSCF TightOpt NoRI Freq CPCM(water)` from the
# MOLS geometries (2026-10-26).  Bond lengths in angstrom, angles in degrees,
# frequencies in cm-1; methanol geometry order C, O, H(anti), H, H, H(O).
OPT = {
    "h2o": ([("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692), ("H", 0, -0.7572, -0.4692)],
            {"OH": 0.9505, "HOH": 104.5076}, [1796.7, 4028.5, 4127.0]),
    "ch3oh": ([("C", -0.0467, 0.6636, 0.0), ("O", -0.0467, -0.7559, 0.0), ("H", -1.0850, 0.9775, 0.0),
               ("H", 0.4402, 1.0680, 0.8903), ("H", 0.4402, 1.0680, -0.8903), ("H", 0.8589, -1.0620, 0.0)],
              {"CO": 1.4063, "OH": 0.9494, "COH": 108.8289},
              [332.9, 1154.2, 1170.8, 1287.1, 1505.3, 1628.6, 1635.5, 1651.8, 3195.4, 3247.3, 3296.3, 4066.7]),
}
TOL_BOND, TOL_ANGLE, TOL_FREQ = 0.002, 0.3, 5.0


def angle(a, b, c):
    u = [x - y for x, y in zip(a, b)]
    v = [x - y for x, y in zip(c, b)]
    return math.degrees(math.acos(sum(x * y for x, y in zip(u, v)) / math.hypot(*u) / math.hypot(*v)))


def solvated_optfreq(exe):
    """optimize + frequency with the solvent: geometry and frequencies against ORCA."""
    bad = 0
    for name, (atoms, geom, freqs) in OPT.items():
        deck = "charge 0\nmultiplicity 1\nmethod hf\nbasis 6-31G*\nepsilon 80.151\nruntype optfreq\nopt_conv tight\n" \
               "geometry\n%send\n" % "".join("%s %s %s %s\n" % a for a in atoms)
        r = run_deck(exe, deck)
        out = r.stdout
        if r.returncode != 0 or "solvent_model" not in out:
            print("%-5s optfreq with solvent FAIL: %s" % (name, r.stderr[-200:]))
            bad += 1
            continue
        blk = out.split("begin geometry")[-1].split("end geometry")[0].splitlines()[1:]
        x = [[float(v) for v in l.split()[2:5]] for l in blk if l.strip()]
        got = {}
        if name == "h2o":
            got = {"OH": math.dist(x[0], x[1]), "HOH": angle(x[1], x[0], x[2])}
        else:
            got = {"CO": math.dist(x[0], x[1]), "OH": math.dist(x[1], x[5]), "COH": angle(x[0], x[1], x[5])}
        vib = re.search(r"begin vibrations.*?\n(.*?)end vibrations", out, re.S).group(1).splitlines()
        fr = [float(l.split()[1]) for l in vib if len(l.split()) >= 2 and l.split()[0].isdigit()]
        fr = [f for f in fr if f > 1.0]
        ok = len(fr) == len(freqs)
        ok = ok and all(abs(got[k] - v) <= (TOL_ANGLE if k in ("HOH", "COH") else TOL_BOND) for k, v in geom.items())
        ok = ok and all(abs(a - b) <= TOL_FREQ for a, b in zip(fr, freqs))
        bad += not ok
        print("%-5s %s  solvated optfreq: %s  freq max dev %.1f cm-1 (ORCA)" % (
            name, "ok  " if ok else "FAIL",
            " ".join("%s %.4f/%.4f" % (k, got[k], v) for k, v in geom.items()),
            max(abs(a - b) for a, b in zip(fr, freqs)) if len(fr) == len(freqs) else -1))
    return bad


def main():
    exe = sys.argv[1]
    bad = 0
    for name, (q, atoms, ref, tol) in MOLS.items():
        d = (energy(exe, q, atoms, "epsilon 80.151\n") - energy(exe, q, atoms, "")) * H_KJ
        ok = abs(d - ref) <= tol
        bad += not ok
        print("%-5s %s  ecce-qm %8.2f  ORCA %8.2f  kJ/mol" % (name, "ok  " if ok else "FAIL", d, ref))
    bad += solvated_optfreq(exe)
    # polarizability has no solvent response: refused, never silently run in vacuum
    r = run_deck(exe, "charge 0\nmultiplicity 1\nmethod hf\nbasis STO-3G\nsolvent water\npolarizability true\ngeometry\nH 0 0 0\nH 0 0 0.74\nend\n")
    refused = r.returncode != 0 and "solvent" in r.stderr and "polarizability" in r.stderr
    print("polarizability with solvent refused:", "ok" if refused else "FAIL")
    bad += not refused
    return 1 if bad else 0


sys.exit(main())
