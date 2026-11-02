#!/usr/bin/env python3
"""Docking (aISS) with xtb dock through the real Launch (#682).

Local data, no data server.  K+ is the first atom of an 18-crown-6 + K+
system, so the receptor/ligand split (automatic: the smallest molecule) and the
restoring of the atom order are both exercised.  The deck is made by the real
ai.xtb, stored with a new calculation and run through Launch, gensub (sub xtb),
eccejobmaster, eccejobstore and eccejobmonitor.  The result must be GEOMTRACE
with one frame of all 43 atoms per pose, and POSESCORE (kcal/mol) with one score
per pose, the K+ at atom 1 in every frame.

xtb 6.7.1 (Debian) aborts in the final optimisation of xtb dock.  When a probe
shows that, a stand-in "xtb" script that writes optimized_structures.xyz in the
format xtb writes is used for the run, and the test says so; the deck, the
split, gensub, the parser and the storage are the real ones either way.

    tests/launch/dock_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without xtb or the built binaries.
"""

import argparse
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

# 18-crown-6, 42 atoms (Angstrom); the K+ starts outside it, as a docking input.
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
ATOMS = [("K", 0.0, 0.0, 7.0)] + CROWN
N = len(ATOMS)

STAND_IN = r"""#!/usr/bin/env python3
import sys
if len(sys.argv) < 4 or sys.argv[1] != "dock":
    sys.exit("stand-in xtb: only 'dock' is supported")
def read(path):
    rows = open(path).read().split("\n")
    return [r.split() for r in rows[2:2 + int(rows[0])]]
a, b = read(sys.argv[2]), read(sys.argv[3])
with open("optimized_structures.xyz", "w") as out:
    # Not in rank order on purpose: the score is what ranks the poses.
    for i, e in enumerate((-20.5, -31.25, -12.0)):
        out.write("%d\n SCF done %.8f\n" % (len(a) + len(b), e))
        for r in a:
            out.write("%s %s %s %s\n" % tuple(r))
        for r in b:
            out.write("%s %.6f %.6f %.6f\n" % (r[0], float(r[1]) + 3.0 + i,
                                                float(r[2]), float(r[3])))
print(" normal termination of xtb")
"""


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(atoms, theory="GFN-FF",
         extra="ES.Runtype.Docking.LigandCharge:\t1\n"):
    """The input deck ai.xtb writes for a system."""
    tmp = tempfile.mkdtemp(prefix="dockdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tdocktest\nTheory:\t%s\nCategory:\tSE\n"
            "RunType:\tDocking (aISS)\nCharge:\t1\n"
            "ES.ChemSys.Multiplicity:\t1\n%s" % (theory, extra))
        frag = ["x", "title:\tdocktest", "type:\tmolecule",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "xtb.xtbin"), "w").write(
            "##XtbHeader##\n##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        res = subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.xtb"),
             "-p", "-f", "-n", "x", "-t", "xtb.xtbin"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return open(os.path.join(tmp, "xtb.xtbin")).read(), res.returncode
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def numbers(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        body = handle.read().split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def realDockWorks(xtb):
    """True if this xtb gets through a small xtb dock run."""
    probe = tempfile.mkdtemp(prefix="dockprobe-")
    try:
        for n, a in (("a.xyz", [("O", 0.0, 0.0, 0.1173), ("H", 0.0, 0.7572, -0.4692),
                                ("H", 0.0, -0.7572, -0.4692)]),
                     ("b.xyz", [("N", 0.0, 0.0, 0.116), ("H", 0.94, 0.0, -0.271),
                                ("H", -0.47, 0.814, -0.271),
                                ("H", -0.47, -0.814, -0.271)])):
            with open(os.path.join(probe, n), "w") as h:
                h.write("%d\n%s\n" % (len(a), n))
                for t in a:
                    h.write("%s %.4f %.4f %.4f\n" % t)
        res = subprocess.run([xtb, "dock", "a.xyz", "b.xyz", "--gfnff", "--fast"],
                             cwd=probe, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, timeout=120)
        poses = os.path.join(probe, "optimized_structures.xyz")
        return (res.returncode == 0 and os.path.exists(poses)
                and os.path.getsize(poses) > 0)
    except Exception:
        return False
    finally:
        shutil.rmtree(probe, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    xtb = shutil.which("xtb")
    if not xtb:
        harness.skip("xtb is not installed")
    harness.prerequisites(build, ())

    say("--- deck")
    text, rc = deck(ATOMS)
    lines = text.splitlines()
    check(rc == 0 and lines[0] == "xtb dock --gfnff --chrg 0 --chrg2 1 --uhf 0 --fast",
          "default: %r" % lines[:1])
    check(lines[1] == "$order" and lines[2].split()[-1] == "1" and
          lines[2].split()[0] == "2" and lines[3] == "$end",
          "ligand K goes last in the atom order: %r" % lines[1:4])
    check("$receptor" in lines and "$ligand" in lines and "42" in lines,
          "receptor and ligand blocks")
    text, rc = deck(ATOMS, "GFN2-xTB",
                    "ES.Theory.Solvent.Model:\tALPB\nES.Theory.Solvent.Name:\twater\n"
                    "ES.Runtype.Docking.LigandAtoms:\t1\n"
                    "ES.Runtype.Docking.LigandCharge:\t1\n"
                    "ES.Runtype.Docking.Fast:\t0\nES.Runtype.Docking.NumPoses:\t6\n")
    check(rc == 0 and text.splitlines()[0] ==
          "xtb dock --gfn2 --chrg 0 --chrg2 1 --uhf 0 --alpb water --nfinal 6",
          "all options: %r" % text.splitlines()[0])
    text, rc = deck(ATOMS[1:])
    check(rc != 0, "one molecule and no ligand atoms is refused")
    text, rc = deck(ATOMS, "GFN1-xTB")
    check(rc != 0, "GFN1-xTB is refused")

    work0 = tempfile.mkdtemp(prefix="dockxtb-")
    real = realDockWorks(xtb)
    prog = xtb
    if not real:
        prog = os.path.join(work0, "xtb")
        with open(prog, "w") as h:
            h.write(STAND_IN)
        os.chmod(prog, 0o755)
        say("  NOTE: xtb dock aborts with this xtb; using a stand-in for the run")

    s = harness.Session(build, "dock", {"xTB": prog}, local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        say("--- 18-crown-6 + K+, GFN-FF")
        text, rc = deck(ATOMS)
        name = "d-%d" % int(time.time())
        path = os.path.join(work, name + ".xtbin")
        open(path, "w").write(text)
        xyz = os.path.join(work, name + ".xyz")
        with open(xyz, "w") as h:
            h.write("%d\n%s\n" % (N, name))
            for a in ATOMS:
                h.write("%s %.6f %.6f %.6f\n" % a)
        url, out = s.create(name, "xtb_es", path, "xtb.xtbin", rundir,
                            extra=("procs=2", "code=xTB", "xyz=" + xyz))
        if not check(url is not None, "calculation created"):
            say(out)
            return 1
        rc, out = s.launch(url)
        check(rc == 0, "Launch ran to the end")
        state = s.waitState(url, seconds=300)
        if not check(state == "completed", "reached completed (last: %s)" % state):
            say(s.driver("reason", url)[1])
            return 1
        time.sleep(2)
        calc = calcDir(url)
        props = s.props("file://" + calc)
        say("  properties: " + " ".join(props))
        for key in ("GEOMTRACE", "POSESCORE"):
            check(key in props, "%s present in Props/" % key)
        score = numbers(calc, "POSESCORE")
        geo = numbers(calc, "GEOMTRACE")
        check(len(score) >= 1, "%d poses stored" % len(score))
        check(len(geo) == len(score) * N * 3,
              "GEOMTRACE has one frame of %d atoms per pose" % N)
        check(all(v < 0 for v in score),
              "interaction energies are negative: %r" % score)
        if not real and len(geo) == 3 * N * 3:
            # The stand-in moves the ligand by 3, 4, 5 A in x and leaves the host.
            check(all(abs(geo[f * N * 3] - (3.0 + f)) < 1e-3 for f in range(3)),
                  "K+ is atom 1 of every frame (moved by the stand-in)")
            check(all(abs(geo[f * N * 3 + 3 + i] - ATOMS[1][1 + i]) < 1e-3
                      for f in range(3) for i in range(3)),
                  "the host keeps its place as atom 2")
    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)
        shutil.rmtree(work0, ignore_errors=True)

    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
