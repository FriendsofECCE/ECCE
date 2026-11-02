#!/usr/bin/env python3
"""xTB "Conformer search (simple)" through the real Launch (#682).

n-butane (GFN2-xTB) must give anti and gauche, gauche 0.5-1.0 kcal/mol above
anti; pentane at least 3 unique conformers.  The ensemble is stored as
GEOMTRACE + CONFREL, like CREST\'s, and the seed is in the output.

    tests/launch/confsearch_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without xtb or python3.
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

WATER = [("O", 0.0, 0.0, 0.1173), ("H", 0.0, 0.7572, -0.4692),
         ("H", 0.0, -0.7572, -0.4692)]
METHANE = [("C", 0.0, 0.0, 0.0), ("H", 0.7, 0.7, 0.7), ("H", -0.7, -0.7, 0.8),
           ("H", -0.7, 0.7, -0.7), ("H", 0.7, -0.8, -0.7)]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def chain(n):
    """An all-anti alkane CnH2n+2 (zigzag), roughly relaxed."""
    import math
    atoms = []
    for i in range(n):
        atoms.append(("C", 1.27 * i, 0.5 * (i % 2), 0.0))
    for i in range(n):
        y = -0.9 if i % 2 == 0 else 1.4
        atoms.append(("H", 1.27 * i, y, 0.89))
        atoms.append(("H", 1.27 * i, y, -0.89))
    atoms.append(("H", -1.0, 0.2, 0.0))
    atoms.append(("H", 1.27 * (n - 1) + 1.0, 0.5 * ((n - 1) % 2) + (0.3 if n % 2 == 0 else -0.3), 0.0))
    return atoms


def relaxed(atoms):
    """The rough chain after a GFN-FF optimisation, so its bonds are sane."""
    tmp = tempfile.mkdtemp(prefix="confstart-")
    try:
        with open(os.path.join(tmp, "a.xyz"), "w") as h:
            h.write("%d\nstart\n" % len(atoms))
            for a in atoms:
                h.write("%s %.6f %.6f %.6f\n" % a)
        subprocess.run(["xtb", "a.xyz", "--gfnff", "--opt", "crude"], cwd=tmp,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        lines = open(os.path.join(tmp, "xtbopt.xyz")).read().splitlines()[2:]
        return [(l.split()[0],) + tuple(float(v) for v in l.split()[1:4])
                for l in lines if l.strip()]
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def deck(atoms, theory="GFN2-xTB", extra=""):
    """The input deck ai.xtb writes for a molecule."""
    tmp = tempfile.mkdtemp(prefix="xtbdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tconf\nTheory:\t%s\nCategory:\tSE\n"
            "RunType:\tConformer search (simple)\nCharge:\t0\n"
            "ES.ChemSys.Multiplicity:\t1\n%s" % (theory, extra))
        frag = ["x", "title:\tconf", "type:\tmolecule",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "xtb.xtbin"), "w").write(
            "##XtbHeader##\n##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.xtb"),
             "-p", "-f", "-n", "x", "-t", "xtb.xtbin"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return open(os.path.join(tmp, "xtb.xtbin")).read()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def numbers(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        body = handle.read().split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def runJob(s, name, text, work, rundir, atoms):
    path = os.path.join(work, name + ".xtbin")
    open(path, "w").write(text)
    xyz = os.path.join(work, name + ".xyz")
    with open(xyz, "w") as h:
        h.write("%d\n%s\n" % (len(atoms), name))
        for a in atoms:
            h.write("%s %.6f %.6f %.6f\n" % a)
    url, out = s.create(name, "xtb_es", path, "xtb.xtbin", rundir,
                        extra=("procs=2", "code=xTB", "xyz=" + xyz))
    if not check(url is not None, "%s created" % name):
        say(out)
        return None
    rc, out = s.launch(url)
    if not check(rc == 0, "%s: Launch ran to the end" % name):
        return None
    state = s.waitState(url, seconds=300)
    if not check(state == "completed",
                 "%s reached completed (last: %s)" % (name, state)):
        say(s.driver("reason", url)[1])
        return None
    time.sleep(2)
    return calcDir(url)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    xtb = shutil.which("xtb")
    if not xtb or not shutil.which("python3"):
        harness.skip("xtb and python3 are not both installed")
    harness.prerequisites(build, ())

    text = deck(chain(4), "GFN-FF")
    check(text.splitlines()[0] == "xtb --gfnff --chrg 0 --uhf 0 --opt normal "
          "--ecce-confsearch samples=30,seed=682,ewin=6,rmsd=0.25",
          "default deck line: %r" % text.splitlines()[0])

    s = harness.Session(build, "confsearch", {"xTB": xtb}, local=True,
                        keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        for name, n, minimum in (("butane", 4, 2), ("pentane", 5, 3)):
            say("--- %s, GFN2-xTB" % name)
            atoms = relaxed(chain(n))
            calc = runJob(s, "%s-%d" % (name, int(time.time())),
                          deck(atoms), work, rundir, atoms)
            if not calc:
                continue
            out = open(os.path.join(calc, "Outputs", "xtb.xtbout"),
                       errors="replace").read()
            check("seed 682" in out, "the seed is recorded in the output")
            props = s.props("file://" + calc)
            for key in ("GEOMTRACE", "CONFREL"):
                check(key in props, "%s present in Props/" % key)
            rel = numbers(calc, "CONFREL")
            geo = numbers(calc, "GEOMTRACE")
            say("  relative energies: %r" % rel)
            check(len(rel) >= minimum, "%d unique conformers (need %d)"
                  % (len(rel), minimum))
            check(len(geo) == len(rel) * len(atoms) * 3,
                  "GEOMTRACE has one frame of %d atoms per conformer" % len(atoms))
            check(rel and rel[0] == 0.0 and rel == sorted(rel),
                  "relative energies start at 0 and ascend")
            if n == 4:
                check(len(rel) >= 2 and 0.5 < rel[1] < 1.0,
                      "gauche is 0.5-1.0 kcal/mol above anti: %r" % rel[1:2])
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
