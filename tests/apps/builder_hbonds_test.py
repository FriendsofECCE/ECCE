#!/usr/bin/env python3
"""
Show Hydrogen Bonds uses the gmx hbond criterion (#978): the Builder script
command "hbonds" switches the lines on and logs how many it drew, and the
count is compared with an independent Python count (donors N/O/F/S,
acceptors N/O/F, D...A <= 3.5 A, H-D...A <= 30 degrees) over the atoms the
Builder logs with "coords".  The water dimer is one bond.  Ethanol has none
(its C-H...O contacts are not counted: C is not a donor).

Same installed tree and isolation as bond_dipoles_test.py.  Exit 77 (skip)
without an install or Xvfb.
"""
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
EX = os.path.join(REPO, "data", "client", "examples")
CASES = [
    ("dimer", "genchem/intermolecular-interactions/ecceqm-h2o_dimer/stored/h2o_dimer", 1),
    ("ethanol", "getting-started/ecceqm-ethanol/stored/ethanol", 0),
]
COV = {"H": 0.31, "C": 0.76, "N": 0.71, "O": 0.66, "F": 0.57, "S": 1.05}


def count(atoms, donors, dist=3.5, angle=30.0):
    """H-bonds by the gmx criterion; H belongs to the nearest atom in
    covalent range, as a bond perception would give."""
    n = 0
    for i, (e, p) in enumerate(atoms):
        if e != "H":
            continue
        near = [(math.dist(p, q), j) for j, (f, q) in enumerate(atoms)
                if j != i and math.dist(p, q) < 0.5 * (COV["H"] + COV.get(f, 1.5)) + 0.45]
        if not near:
            continue
        d = min(near)[1]
        if atoms[d][0] not in donors:
            continue
        pd = atoms[d][1]
        for j, (f, q) in enumerate(atoms):
            if f not in ("N", "O", "F") or j == d:
                continue
            if math.dist(pd, q) < 1.7 or math.dist(pd, q) > dist:
                continue
            dh = [a - b for a, b in zip(p, pd)]
            da = [a - b for a, b in zip(q, pd)]
            cos = sum(a * b for a, b in zip(dh, da)) / (math.hypot(*dh) * math.hypot(*da))
            if cos >= math.cos(math.radians(angle)):
                n += 1
    return n


def runBuilder(display, url, out):
    os.makedirs(out, exist_ok=True)
    lines = ["wait 3000", "coords", "hbonds", "wait 500",
             "hbonds 3.0 20", "wait 500", "hbonds 3.5 30", "quit"]
    with open(os.path.join(out, "script"), "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(out, "builder.log")
    env = display.env()
    env.update(ECCE_BUILDER_SCRIPT=os.path.join(out, "script"))
    proc = subprocess.Popen([os.path.join(apps.WRAPPERS, "ecce-builder"),
                             "-context", url], env=env,
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    try:
        proc.wait(timeout=240)
    except subprocess.TimeoutExpired:
        pass
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, 9)
    return open(log, errors="replace").read()


def main():
    if "ECCE_TEST_WRAPPERS" not in os.environ:
        print("SKIP: set ECCE_TEST_HOME and ECCE_TEST_WRAPPERS to a private install")
        return 77
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="hbonds-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        for name, rel, want in CASES:
            url = base.makeCalc(data, name, source=os.path.join(EX, rel))
            log = runBuilder(display, url, os.path.join(work, name))
            atoms = [(m.group(1), [float(m.group(i)) for i in (2, 3, 4)])
                     for m in re.finditer(
                         r"BUILDER: atom \d+ (\S+) (\S+) (\S+) (\S+)", log)]
            got = [int(v) for v in re.findall(r"BUILDER: hbonds=(\d+)", log)]
            check(len(atoms) > 2 and len(got) == 3,
                  "%s: %d atoms, hbonds logged %s" % (name, len(atoms), got))
            if len(atoms) <= 2 or len(got) != 3:
                print(log[-1500:])
                continue
            mine = count(atoms, "NOFS")
            withC = count(atoms, "NOFSC")
            print("     %s: Builder %d, independent count %d, with C donors %d"
                  % (name, got[0], mine, withC))
            check(got[0] == mine, "%s: Builder count equals the independent one" % name)
            check(got[0] == want, "%s: %d hydrogen bond(s)" % (name, want))
            check(got[2] == got[0], "%s: default criteria restored by hbonds 3.5 30" % name)
            check(got[1] <= got[0], "%s: tighter criteria never add bonds" % name)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
