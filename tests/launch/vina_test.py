#!/usr/bin/env python3
"""AutoDock Vina docking through the real Launch (#455).

Local data, no data server.  18-crown-6 is the receptor and ethanol (the last
nine atoms, so the automatic split picks the smaller molecule) the ligand.  The
deck is made by the real ai.vina: Vina's configuration with a sane search box
and the structure carried as comment lines.  With vina and ecce-pdbqt present
the deck is stored with a new calculation and run through Launch, gensub (sub
vina), eccejobmaster, eccejobstore and eccejobmonitor.  The result must be
GEOMTRACE with one frame of all 51 atoms per pose, in the calculation's atom
order with the receptor where it was, and POSESCORE (kcal/mol), best first.

    tests/launch/vina_test.py [--build build-cmake] [--keep]

The deck checks always run.  Exit status 77 (CTest SKIP) after them without
vina, ecce-pdbqt (or ECCE_PDBQT) or the built binaries.
"""

import argparse
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []

# 18-crown-6, 42 atoms (Angstrom).
CROWN = [
    ("C", -1.38113, 1.72307, 0.25375), ("C", -0.13983, 2.26398, -0.44409),
    ("O", 1.00270, 1.86493, 0.30630), ("C", 2.20683, 2.45866, -0.16897),
    ("C", 3.38943, 1.89367, 0.62713), ("O", 3.48382, 0.46422, 0.59932),
    ("C", 3.79128, -0.07519, -0.68385), ("C", 3.50318, -1.57734, -0.71807),
    ("O", 2.11049, -1.90499, -0.67981), ("C", 1.54665, -1.83059, 0.63773),
    ("C", 0.26110, -2.65926, 0.69337), ("O", -0.80175, -1.97100, 0.03224),
    ("C", -2.05334, -2.65119, 0.17736), ("C", -3.14754, -1.81021, -0.47760),
    ("O", -3.36917, -0.64487, 0.32266), ("C", -4.12065, 0.35923, -0.36025),
    ("C", -3.76201, 1.73318, 0.21500), ("O", -2.55516, 2.24814, -0.36789),
    ("H", -1.36352, 2.00965, 1.31076), ("H", -1.35633, 0.63281, 0.20333),
    ("H", -0.19342, 3.35853, -0.50168), ("H", -0.07949, 1.87473, -1.46632),
    ("H", 2.15116, 3.54288, -0.01670), ("H", 2.31774, 2.27041, -1.24095),
    ("H", 3.25504, 2.16084, 1.68211), ("H", 4.33421, 2.33220, 0.29029),
    ("H", 4.85451, 0.09788, -0.87633), ("H", 3.20441, 0.40795, -1.46961),
    ("H", 4.03175, -2.10315, 0.08526), ("H", 3.87442, -1.97065, -1.66994),
    ("H", 1.33259, -0.78959, 0.88910), ("H", 2.25575, -2.24971, 1.36160),
    ("H", -0.01522, -2.81971, 1.74120), ("H", 0.41247, -3.62632, 0.20140),
    ("H", -2.27072, -2.78625, 1.24299), ("H", -1.98565, -3.62680, -0.31524),
    ("H", -4.08310, -2.37460, -0.53605), ("H", -2.82539, -1.51919, -1.48388),
    ("H", -5.18062, 0.14771, -0.19378), ("H", -3.91043, 0.34868, -1.43494),
    ("H", -3.68092, 1.71505, 1.30655), ("H", -4.55082, 2.44132, -0.05771)]
# Ethanol, a few A above the ring (clear of bonding distance).
ETHANOL = [("C", 1.00048, 0.09744, 5.0), ("C", 2.51477, 0.09677, 4.99),
           ("O", 2.99645, 0.71814, 3.80), ("H", 0.61568, -0.36965, 5.91),
           ("H", 0.60848, -0.44486, 4.13), ("H", 0.61500, 1.12040, 4.93),
           ("H", 2.90619, -0.92417, 5.04), ("H", 2.90176, 0.65256, 5.85),
           ("H", 2.65687, 0.21117, 3.05)]
ATOMS = CROWN + ETHANOL
N = len(ATOMS)
NREC = len(CROWN)


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(atoms=ATOMS, extra=""):
    """The input deck ai.vina writes for a system."""
    tmp = tempfile.mkdtemp(prefix="vinadeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tvinatest\nTheory:\tVina\nCategory:\tSE\n"
            "RunType:\tDocking (Vina)\nCharge:\t0\n"
            "ES.ChemSys.Multiplicity:\t1\n%s" % extra)
        frag = ["x", "title:\tvinatest", "type:\tmolecule",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "vina.vinain"), "w").write("##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        res = subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.vina"),
             "-p", "-f", "-n", "x", "-t", "vina.vinain"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        frag = os.path.join(tmp, "vina.vinafrag")
        deck.structure = open(frag).read() if os.path.exists(frag) else ""
        return open(os.path.join(tmp, "vina.vinain")).read(), res.returncode
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def config(text):
    """The key = value lines of a deck."""
    out = {}
    for line in text.splitlines():
        m = re.match(r"^(\w+)\s*=\s*(\S+)", line)
        if m:
            out[m.group(1)] = m.group(2)
    return out


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def numbers(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        body = handle.read().split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def deckChecks():
    say("--- deck")
    text, rc = deck()
    cfg = config(text)
    check(rc == 0, "ai.vina ran")
    check(cfg.get("receptor") == "receptor.pdbqt" and
          cfg.get("ligand") == "ligand.pdbqt", "receptor and ligand files")
    ligand = ETHANOL
    lo = [min(a[k] for a in ligand) for k in (1, 2, 3)]
    hi = [max(a[k] for a in ligand) for k in (1, 2, 3)]
    centre = [(lo[k] + hi[k]) / 2 for k in range(3)]
    try:
        got = [float(cfg["center_" + c]) for c in "xyz"]
        size = [float(cfg["size_" + c]) for c in "xyz"]
    except KeyError:
        got, size = [99.0] * 3, [0.0] * 3
    check(all(abs(got[k] - centre[k]) <= 0.06 for k in range(3)),
          "the box is centred on the ligand: %r vs %r" % (got, centre))
    check(all(size[k] >= 16.0 and abs(size[k] - max(16.0, hi[k] - lo[k] + 10)) <= 0.06
              for k in range(3)), "box size is the ligand plus a margin, at least 16: %r" % size)
    check((cfg.get("exhaustiveness"), cfg.get("num_modes"), cfg.get("energy_range"))
          == ("8", "9", "3") and "seed" not in cfg, "defaults: %r" % cfg)
    check("#| LIGAND_ATOMS=%d-%d\n" % (NREC + 1, N) in text,
          "ligand atoms are the last nine, as a range")
    check("#| RECEPTOR_ATOMS=1-%d\n" % NREC in text and "BEGIN-FRAG" not in text,
          "receptor atoms are a range and the structure is not in the deck")
    check(deck.structure.count("\n") >= N + 6 and "num_atoms:\t%d" % N in deck.structure,
          "the structure is the auxiliary input vina.vinafrag")

    text, rc = deck(extra=(
        "ES.Runtype.Docking.LigandAtoms:\t43-51\n"
        "ES.Runtype.Vina.BoxAuto:\t0\n"
        "ES.Runtype.Vina.BoxCenterX:\t1.5\nES.Runtype.Vina.BoxCenterY:\t-2\n"
        "ES.Runtype.Vina.BoxCenterZ:\t3.5\nES.Runtype.Vina.BoxSizeX:\t22\n"
        "ES.Runtype.Vina.BoxSizeY:\t23.5\nES.Runtype.Vina.BoxSizeZ:\t24\n"
        "ES.Runtype.Vina.Exhaustiveness:\t16\nES.Runtype.Vina.NumPoses:\t5\n"
        "ES.Runtype.Vina.EnergyRange:\t2.5\nES.Runtype.Vina.Seed:\t11\n"
        "ES.Runtype.Vina.FlexibleAmides:\t1\n"))
    cfg = config(text)
    check(rc == 0 and [cfg.get(k) for k in (
        "center_x", "center_y", "center_z", "size_x", "size_y", "size_z")] ==
        ["1.5", "-2.0", "3.5", "22.0", "23.5", "24.0"],
        "a given box is used as it is: %r" % cfg)
    check((cfg.get("exhaustiveness"), cfg.get("num_modes"), cfg.get("energy_range"),
           cfg.get("seed")) == ("16", "5", "2.5", "11"), "search options")
    check("#| FLEXIBLE_AMIDES=1" in text, "flexible amides are passed on")

    text, rc = deck(ATOMS[:NREC])
    check(rc != 0, "one molecule and no ligand atoms is refused")
    text, rc = deck(extra="ES.Runtype.Vina.BoxAuto:\t0\nES.Runtype.Vina.BoxSizeX:\t-3\n")
    check(rc != 0, "a negative box size is refused")
    text, rc = deck(extra="ES.Runtype.Vina.Exhaustiveness:\t0\n")
    check(rc != 0, "exhaustiveness 0 is refused")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    deckChecks()
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1

    vina = shutil.which("vina")
    pdbqt = os.environ.get("ECCE_PDBQT") or os.path.join(build, "ecce-pdbqt")
    if not vina:
        harness.skip("vina is not installed")
    if not os.access(pdbqt, os.X_OK):
        harness.skip("ecce-pdbqt is not built in %s" % build)
    harness.prerequisites(build, ())
    #  The job script takes ecce-pdbqt from here, else from its ECCE_HOME.
    os.environ["ECCE_PDBQT"] = pdbqt

    s = harness.Session(build, "vina", {"Vina": vina}, local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        say("--- 18-crown-6 + ethanol")
        text, rc = deck(extra="ES.Runtype.Vina.Seed:\t7\nES.Runtype.Vina.NumPoses:\t4\n"
                        "ES.Runtype.Vina.Exhaustiveness:\t4\n")
        name = "v-%d" % int(time.time())
        path = os.path.join(work, name + ".vinain")
        open(path, "w").write(text)
        structure = os.path.join(work, "vina.vinafrag")
        open(structure, "w").write(deck.structure)
        xyz = os.path.join(work, name + ".xyz")
        with open(xyz, "w") as h:
            h.write("%d\n%s\n" % (N, name))
            for a in ATOMS:
                h.write("%s %.6f %.6f %.6f\n" % a)
        url, out = s.create(name, "vina_es", path, "vina.vinain", rundir,
                            extra=("procs=2", "code=Vina", "xyz=" + xyz,
                                   "input=" + structure))
        if not check(url is not None, "calculation created"):
            say(out)
            return 1
        rc, out = s.launch(url)
        check(rc == 0, "Launch ran to the end")
        state = s.waitState(url, seconds=600)
        if not check(state == "completed", "reached completed (last: %s)" % state):
            say(s.driver("reason", url)[1])
            return 1
        time.sleep(2)
        calc = calcDir(url)
        outFile = os.path.join(calc, "Outputs", "vina.vinaout")
        check(os.path.exists(outFile), "the output was stored as Outputs/vina.vinaout")
        props = s.props("file://" + calc)
        say("  properties: " + " ".join(props))
        for key in ("GEOMTRACE", "POSESCORE"):
            check(key in props, "%s present in Props/" % key)
        score = numbers(calc, "POSESCORE")
        geo = numbers(calc, "GEOMTRACE")
        check(1 <= len(score) <= 4, "%d poses stored" % len(score))
        check(len(geo) == len(score) * N * 3,
              "GEOMTRACE has one frame of %d atoms per pose" % N)
        check(score == sorted(score), "best pose first: %r" % score)
        if len(geo) == len(score) * N * 3:
            frames = [geo[f * N * 3:(f + 1) * N * 3] for f in range(len(score))]
            check(all(abs(fr[3 * i + k] - ATOMS[i][1 + k]) < 1e-3
                      for fr in frames for i in range(NREC) for k in range(3)),
                  "the receptor keeps its place in every frame")
            def d(fr, a, b):
                return math.dist(fr[3 * a:3 * a + 3], fr[3 * b:3 * b + 3])
            check(all(1.3 < d(fr, NREC, NREC + 1) < 1.7 and
                      1.3 < d(fr, NREC + 1, NREC + 2) < 1.6 and
                      0.9 < d(fr, NREC + 2, NREC + 8) < 1.1 for fr in frames),
                  "ligand atoms are in the calculation's order (C-C, C-O, O-H hold)")
    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
