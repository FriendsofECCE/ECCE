#!/usr/bin/env python3
"""Mode symmetry and description (#367) against textbook assignments.

    tests/vibanalysis/run_tests.py --cli build-cmake/vibanalysis_cli [--build build-cmake] [-v]

Real output of the codes ECCE reads goes through the real
parsers (tests/parsers) to a modes file, vibanalysis_cli analyses it, and the
irreps, IR/Raman activity and short descriptions are compared with what the
textbooks give for H2O, CO2, NH3, CH4, HCHO and benzene.  Where a code prints
its own mode symmetry, ECCE's labels are compared with it
too, after the one axis-naming difference those codes may have (b1/b2 of a
planar C2v molecule).  The expected values here are written from the textbook
and the experimental assignments, not produced by the analysis.

Exit 77 (skip) without the CLI, autosym or symops; 1 on a failure.
"""
import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import extract  # noqa: E402

FIX = os.path.join(REPO, "tests", "parsers", "fixtures")
QM = os.path.join(HERE, "fixtures")

NU, DELTA, RHO, OMEGA = "ν", "δ", "ρ", "ω"
SIGMA, PI = "Σ", "Π"
MINUS = "−"

# (irrep, IR active, Raman active, short label or the labels accepted) of the
# vibrations.  The order of the list does not matter: where the modes fall in
# frequency depends on the method (PM7 puts methane's e bend below its t2).
TEXTBOOK = {
    "H2O": [("A1", True, True, DELTA + "(HOH)"),
            ("A1", True, True, NU + "s(OH)"),
            ("B2", True, True, NU + "as(OH)")],
    "CO2": [(PI + "u", True, False, DELTA + "(OCO)"),
            (PI + "u", True, False, DELTA + "(OCO)"),
            (SIGMA + "g+", False, True, NU + "s(C=O)"),
            (SIGMA + "u+", True, False, NU + "as(C=O)")],
    "NH3": [("A1", True, True, DELTA + "s(HNH)"),
            ("E", True, True, DELTA + "as(HNH)"),
            ("E", True, True, DELTA + "as(HNH)"),
            ("A1", True, True, NU + "s(NH)"),
            ("E", True, True, NU + "as(NH)"),
            ("E", True, True, NU + "as(NH)")],
    "CH4": [("T2", True, True, DELTA + "as(HCH)")] * 3 +
           [("E", False, True, DELTA + "as(HCH)")] * 2 +
           [("A1", False, True, NU + "s(CH)")] +
           [("T2", True, True, NU + "as(CH)")] * 3,
    "HCHO": [("B1", True, True, OMEGA + "(CH2)"),
             ("B2", True, True, RHO + "(CH2)"),
             ("A1", True, True, (DELTA + "(HCH)", DELTA + "(HCH) + " + NU + "(C=O)",
                                 NU + "(C=O) + " + DELTA + "(HCH)")),
             ("A1", True, True, (NU + "(C=O)", NU + "(C=O) + " + DELTA + "(HCH)",
                                 DELTA + "(HCH) + " + NU + "(C=O)")),
             ("A1", True, True, NU + "s(CH)"),
             ("B2", True, True, NU + "as(CH)")],
}

# Benzene, D6h: 2 a1g + a2g + a2u + 2 b2g + 2 b1u + 2 b2u + e1g + 3 e1u +
# 4 e2g + 2 e2u.  Infrared: a2u and e1u only; Raman: a1g, e1g, e2g only.
BENZENE = {"A1g": 2, "A2g": 1, "A2u": 1, "B2g": 2, "B1u": 2, "B2u": 2,
           "E1g": 2, "E1u": 6, "E2g": 8, "E2u": 4}
BENZENE_IR = {"A2u", "E1u"}
BENZENE_RAMAN = {"A1g", "E1g", "E2g"}

BONDS = {
    "CO2": [(0, 1, 2), (0, 2, 2)],
    "HCHO": [(0, 1, 2), (0, 2, 1), (0, 3, 1)],
    "BENZENE": [(0, 1, 1.5), (1, 2, 1.5), (2, 3, 1.5), (3, 4, 1.5), (4, 5, 1.5),
                (5, 0, 1.5), (0, 6, 1), (1, 7, 1), (2, 8, 1), (3, 9, 1),
                (4, 10, 1), (5, 11, 1)],
}

# molecule, label, fixture, desc, runtype, element override
CASES = [
    ("H2O", "ORCA", os.path.join(FIX, "orca/h2o_freq_raman.out"), "orca.desc", "Vibration", None),
    ("H2O", "NWChem", os.path.join(FIX, "nwchem/h2o_freq.eprint"), "nwchem.desc", "Vibration", "OHH"),
    ("H2O", "Gaussian", os.path.join(FIX, "gaussian-16/h2o_optfreq.log"), "gaussian-16.desc", "GeoVib", None),
    ("H2O", "ECCE-QM", os.path.join(FIX, "ecceqm/h2o_optfreq.qmout"), "ecceqm.desc", "GeoVib", "OHH"),
    ("CH4", "ORCA", os.path.join(FIX, "orca/ch4_freq_raman.out"), "orca.desc", "Vibration", None),
    ("CH4", "MOPAC", os.path.join(FIX, "mopac/ch4_force.out"), "mopac.desc", "Vibration", None),
    ("CH4", "ECCE-QM", os.path.join(QM, "ch4_optfreq.qmout"), "ecceqm.desc", "GeoVib", "CHHHH"),
    ("NH3", "Gaussian", os.path.join(REPO, "tests/progress/fixtures/gaussian-16/nh3_optfreq.log"),
     "gaussian-16.desc", "GeoVib", None),
    ("NH3", "ECCE-QM", os.path.join(QM, "nh3_optfreq.qmout"), "ecceqm.desc", "GeoVib", "NHHH"),
    ("CO2", "ECCE-QM", os.path.join(QM, "co2_optfreq.qmout"), "ecceqm.desc", "GeoVib", "COO"),
    ("HCHO", "ECCE-QM", os.path.join(QM, "hcho_optfreq.qmout"), "ecceqm.desc", "GeoVib", "COHH"),
    ("BENZENE", "ECCE-QM", os.path.join(QM, "c6h6_optfreq.qmout"), "ecceqm.desc", "GeoVib",
     "CCCCCCHHHHHH"),
]

failures = []
checks = [0]


def check(ok, text):
    checks[0] += 1
    print(("ok   " if ok else "FAIL ") + text)
    if not ok:
        failures.append(text)
    return ok


def run(cli, env, text, name):
    with tempfile.TemporaryDirectory(prefix="vibanalysis-") as tmp:
        path = os.path.join(tmp, "modes.txt")
        with open(path, "w") as f:
            f.write(text)
        proc = subprocess.run([cli, path], capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        check(False, "%s: analysis ran: %s" % (name, proc.stderr.strip()))
        return None
    group, note, modes = "", "", []
    for line in proc.stdout.splitlines():
        if line.startswith("group "):
            group = line.split(None, 1)[1]
        elif line.startswith("note "):
            note = line[5:]
        else:
            f = line.split("\t")
            modes.append(dict(n=int(f[0]), freq=float(f[1]), irrep=f[2], ir=f[3],
                              raman=f[4], label=f[5], detail=f[6]))
    return group, note, modes


def canon(label):
    """A code's irrep spelling, comparable with ours: case, quotes, MOPAC's
    leading count ("1T2") and the names of a linear molecule's groups."""
    s = label.upper().replace('"', "''")
    s = s.lstrip("0123456789")
    table = {"SGG": SIGMA.upper() + "G+", "SGU": SIGMA.upper() + "U+",
             "PIU": PI.upper() + "U", "PIG": PI.upper() + "G"}
    return table.get(s, s)


def compare_codes(name, ours, theirs):
    """Our irreps against the code's own labels.  A code that symmetry-adapts
    in the subgroup Cs prints A'/A'': then ours must subduce to it (A1 -> A',
    A2 -> A'', B -> either, an E pair -> one of each)."""
    vib = [m for m in ours if abs(m["freq"]) >= 10]
    theirs = [canon(t) for t in theirs if t not in ("", "-")]
    theirs = theirs[-len(vib):]
    if len(theirs) != len(vib):
        check(False, "%s: the code printed %d labels for %d vibrations"
              % (name, len(theirs), len(vib)))
        return
    if all(t in ("A'", "A''") for t in theirs):
        ok = True
        i = 0
        while i < len(vib):
            irrep = vib[i]["irrep"].upper()
            if irrep.startswith("E") and i + 1 < len(vib):
                ok &= sorted(theirs[i:i + 2]) == ["A'", "A''"]
                i += 2
                continue
            allowed = {"A1": ["A'"], "A2": ["A''"], "B1": ["A'", "A''"],
                       "B2": ["A'", "A''"]}.get(irrep, [])
            ok &= theirs[i] in allowed
            i += 1
        check(ok, "%s: ECCE's irreps subduce to the code's own Cs labels %s" % (name, theirs))
        return
    same = [m["irrep"].upper() for m in vib] == theirs
    check(same, "%s: ECCE's irreps equal the code's own %s" % (name, theirs))


def match_textbook(name, vib, want):
    """The modes (dicts with irrep, ir, raman, label) against the textbook
    list, in any order."""
    check(len(vib) == len(want), "%s: %d vibrations (textbook %d)" % (name, len(vib), len(want)))
    left = list(vib)
    for irrep, ir, raman, label in want:
        labels = label if isinstance(label, tuple) else (label,)
        hit = None
        for m in left:
            if (m["irrep"] == irrep and (m["ir"] == "IR") == ir and
                    (m["raman"] == "Raman") == raman and m["label"] in labels):
                hit = m
                break
        if hit is not None:
            left.remove(hit)
        check(hit is not None, "%s: a mode %s, IR %s, Raman %s, %s" %
              (name, irrep, ir, raman, " or ".join(labels)))
    for m in left:
        check(False, "%s: mode %d (%.0f cm-1) %s %s %s %s is not in the textbook list"
              % (name, m["n"], m["freq"], m["irrep"], m["ir"], m["raman"], m["label"]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--cli", required=True)
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    cli = os.path.abspath(args.cli)
    build = os.path.abspath(args.build)
    for exe in (cli, os.path.join(build, "autosym"), os.path.join(build, "symops")):
        if not os.access(exe, os.X_OK):
            print("skipped: %s is not built" % exe)
            return 77

    home = tempfile.mkdtemp(prefix="vibanalysis-home-")
    env = dict(os.environ, ECCE_HOME=REPO, HOME=home, ECCE_REALUSERHOME=home,
               ECCE_REALUSER="vibanalysis",
               PATH=build + os.pathsep + os.environ.get("PATH", ""))

    table = {}
    for mol, code, fixture, desc, runtype, elements in CASES:
        name = "%s, %s" % (mol, code)
        if not os.path.exists(fixture):
            check(False, "%s: fixture %s exists" % (name, fixture))
            continue
        geometry = None
        if code == "MOPAC":
            #  MOPAC prints its modes in the orientation of its force run, not
            #  the one ECCE stores: the stored one must be refused, and the
            #  modes analysed in their own frame.
            stored, _s, _f = extract.modes_file(fixture, desc, runtype, None, None)
            got = run(cli, env, stored, name + " (stored geometry)")
            if got is not None:
                check(got[0] == "" or got[0] == "-", "%s: modes in another orientation than the stored structure are not labelled (%s)"
                      % (name, got[1]))
                check(all(m["irrep"] == "-" and m["label"] == "-" for m in got[2]),
                      "%s: and nothing is said about them" % name)
            geometry = extract.mopac_force_geometry(fixture)
        text, codeSym, freqs = extract.modes_file(
            fixture, desc, runtype, list(elements) if elements else None, BONDS.get(mol),
            geometry)
        got = run(cli, env, text, name)
        if got is None:
            continue
        group, note, modes = got
        table[name] = (group, modes)
        vib = [m for m in modes if abs(m["freq"]) >= 10]
        if args.verbose:
            print("  %s: group %s %s" % (name, group, note))
            for m in modes:
                print("    %d %8.2f %-5s %-3s %-5s %-14s %s" % (
                    m["n"], m["freq"], m["irrep"], m["ir"], m["raman"], m["label"], m["detail"]))
        if mol == "BENZENE":
            counts = {}
            for m in vib:
                counts[m["irrep"]] = counts.get(m["irrep"], 0) + 1
            check(group.upper() in ("D6H",), "%s: point group %s is D6h" % (name, group))
            check(counts == BENZENE, "%s: vibrations span 2A1g+A2g+A2u+2B2g+2B1u+2B2u+E1g+3E1u+4E2g+2E2u (got %s)"
                  % (name, counts))
            ir = {m["irrep"] for m in vib if m["ir"] == "IR"}
            ra = {m["irrep"] for m in vib if m["raman"] == "Raman"}
            check(ir == BENZENE_IR, "%s: IR active %s, textbook %s" % (name, sorted(ir), sorted(BENZENE_IR)))
            check(ra == BENZENE_RAMAN, "%s: Raman active %s, textbook %s" % (name, sorted(ra), sorted(BENZENE_RAMAN)))
            for irrep, label in (("A2u", "γ(CH)"),):
                have = [m["label"] for m in vib if m["irrep"] == irrep]
                check(have == [label], "%s: %s mode is %s (got %s)" % (name, irrep, label, have))
            a1g = sorted((m for m in vib if m["irrep"] == "A1g"), key=lambda m: m["freq"])
            if len(a1g) == 2:
                check(a1g[0]["label"] == NU + "s(CC)", "%s: ring breathing a1g is %ss(CC) (got %s)"
                      % (name, NU, a1g[0]["label"]))
                check(a1g[1]["label"] == NU + "s(CH)", "%s: CH stretch a1g is %ss(CH) (got %s)"
                      % (name, NU, a1g[1]["label"]))
            continue
        match_textbook(name, vib, TEXTBOOK[mol])
        if codeSym:
            compare_codes(name, modes, codeSym)

    #  Control: the structure turned about z while the modes stay put is a
    #  frame disagreement and must be refused, not labelled.
    text, _sym, _freqs = extract.modes_file(
        os.path.join(FIX, "orca/ch4_freq_raman.out"), "orca.desc", "Vibration")
    lines = text.splitlines()
    for i in range(1, int(lines[0]) + 1):
        el, x, y, z = lines[i].split()
        lines[i] = "%s %.8f %.8f %.8f" % (el, -float(y), float(x), float(z))
    turned = run(cli, env, "\n".join(lines) + "\n", "CH4 turned")
    check(turned is not None and all(m["irrep"] == "-" and m["label"] == "-" for m in turned[2]),
          "a structure turned against its modes is not labelled (%s)" % (turned and turned[1]))

    #  The same molecule gives the same irreps whichever code's modes it gets.
    h2o = [v for k, v in table.items() if k.startswith("H2O")]
    labels = {tuple(m["irrep"] for m in v[1] if abs(m["freq"]) >= 10) for v in h2o}
    check(len(labels) <= 1, "H2O irreps are the same for every code's output: %s" % sorted(labels))

    print("%d checks, %d failed" % (checks[0], len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
