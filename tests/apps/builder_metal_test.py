#!/usr/bin/env python3
"""Builder: metal-pi attachment, Clean, deleting a nub, Ctrl+A (#325, #321).

    tests/apps/builder_metal_test.py            own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on benzene.car, with the same
commands the GUI runs: Bond mode with the Metal order on a picked metal and
ligand atom (either pick first), the viewer's Clean, the Delete key on a
selected nub, and the Ctrl+A accelerator.
"""
import math
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(ROOT, "tests", "fragreaders", "data", "benzene.car")


def tail(log, n=30):
    return "\n".join("      | " + l for l in log.splitlines()[-n:])


def run(display, name, script, timeout=120):
    work = os.path.join(fixture.stateHome(), "builder-metal-" + name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "metal.script")
    with open(path, "w") as handle:
        handle.write("\n".join(script) + "\n")
    try:
        result = apps.run(display, "builder", args=(STRUCTURE,),
                          windowTimeout=60, settle=timeout,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1",
                               "ECCE_TEST_XYZ_UNITS": "angstrom"})
        return result.log or ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def blocks(log):
    """The atoms of each 'coords' dump: [[(symbol, x, y, z)]]."""
    out, cur = [], []
    for line in log.splitlines():
        m = re.search(r"BUILDER: atom \d+ (\w+) (\S+) (\S+) (\S+)", line)
        if m:
            cur.append((m.group(1),) + tuple(float(v) for v in m.groups()[1:]))
        elif "BUILDER: formula=" in line and cur:
            out.append(cur)
            cur = []
    return out


def sub(a, b):
    return [x - y for x, y in zip(a, b)]


def centroid(pts):
    return [sum(p[k] for p in pts) / len(pts) for k in range(3)]


def ringFacts(atoms, last):
    """Fe-centroid distance and out-of-plane deviation of the last LAST
    carbons, and the Fe position relative to the centroid."""
    fe = [a[1:] for a in atoms if a[0] == "Fe"][0]
    ring = [a[1:] for a in atoms if a[0] == "C"][-last:]
    cen = centroid(ring)
    u, v = sub(ring[0], cen), sub(ring[2], cen)
    n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2],
         u[0] * v[1] - u[1] * v[0]]
    nl = math.sqrt(sum(x * x for x in n)) or 1.0
    dev = max(abs(sum(a * b for a, b in zip(sub(p, cen), n)) / nl)
              for p in ring)
    return math.dist(fe, cen), dev, sub(fe, cen)


def counts(log):
    return [(int(a), int(n)) for a, n in re.findall(
        r"BUILDER: atoms=(\d+) lattice=\w+ nubs=(\d+)", log)]


def piScript(first, second):
    return ["wait 3000", "add Fe Tetrahedral 8 0 0", "bondkind metal",
            "mode bond", "selectelem " + first, "addelem " + second,
            "info", "click", "info", "bonds Fe C", "bonds Fe Nub",
            "coords", "clean", "coords", "info", "quit"]


def piCase(display, results, expect):
    runs = {}
    for label, (a, b) in (("metal first", ("Fe", "C")),
                          ("ligand first", ("C", "Fe"))):
        log = run(display, "pi", piScript(a, b))
        runs[label] = log
        n = counts(log)
        bl = blocks(log)
        bonds = re.findall(r"BUILDER: bonds Fe-(\w+)=(\d+)", log)
        if len(n) != 3 or len(bl) != 2 or len(bonds) != 2:
            expect(False, "%s: script ran (%d info, %d dumps, %d bond "
                   "counts)\n%s" % (label, len(n), len(bl), len(bonds),
                                    tail(log)))
            continue
        expect(n[1][0] == n[0][0] - 1 and n[1][1] == 3,
               "%s: benzene attaches at the Fe nub nearest it, which alone "
               "is consumed (atoms %d -> %d, nubs %d -> %d)"
               % (label, n[0][0], n[1][0], n[0][1], n[1][1]))
        expect(bonds == [("C", "6"), ("Nub", "3")],
               "%s: six Fe-C Metal bonds, three nubs left (%s)"
               % (label, bonds))
        d0, dev0, _ = ringFacts(bl[0], 6)
        d1, dev1, _ = ringFacts(bl[1], 6)
        expect(abs(d0 - 1.7) < 0.05, "%s: Fe over the ring at %.2f A"
               % (label, d0))
        expect(dev1 < 0.05 and abs(d1 - d0) < 0.2,
               "%s: Clean keeps the ring flat (%.3f A) and Fe in place "
               "(%.2f -> %.2f A)" % (label, dev1, d0, d1))
    if len(runs) == 2:
        f = [ringFacts(blocks(l)[0], 6) if len(blocks(l)) == 2 else None
             for l in runs.values()]
        if f[0] and f[1]:
            expect(abs(f[0][0] - f[1][0]) < 0.01
                   and math.dist(f[0][2], f[1][2]) < 0.01,
                   "the result does not depend on which atom was picked "
                   "first (%.3f vs %.3f A)" % (f[0][0], f[1][0]))


def cpCase(display, results, expect):
    log = run(display, "cp", [
        "wait 3000", "add Fe Tetrahedral 8 0 0", "select -1",
        "library Ligands Cyclopentadienyl", "click",
        "info", "bondkind metal", "mode bond", "selectelem Fe",
        "addelem C", "click", "info", "bonds Fe C", "coords", "clean",
        "coords", "info", "quit"])
    n = counts(log)
    bl = blocks(log)
    bonds = re.findall(r"BUILDER: bonds Fe-(\w+)=(\d+)", log)
    if len(n) != 3 or len(bl) != 2 or not bonds:
        expect(False, "Cp: script ran (%d info, %d dumps)\n%s"
               % (len(n), len(bl), tail(log)))
        return
    expect(bonds[0] == ("C", "5"), "Cp: five Fe-C Metal bonds (%s)" % bonds)
    expect(n[1][1] == 3, "Cp: no nub left on the ring, three on Fe "
           "(nubs %d -> %d)" % (n[0][1], n[1][1]))
    d0, _, _ = ringFacts(bl[0], 5)
    d1, dev1, _ = ringFacts(bl[1], 5)
    expect(dev1 < 0.05 and abs(d1 - d0) < 0.2,
           "Cp: Clean keeps the ring flat (%.3f A) and Fe in place "
           "(%.2f -> %.2f A)" % (dev1, d0, d1))


def editCase(display, results, expect):
    log = run(display, "edit", [
        "wait 3000", "add Fe Octahedral 8 0 0", "info", "selectnub Fe",
        "key delete", "info", "select -1", "key viewer selectall", "info",
        "select -1", "key text selectall", "quit"])
    n = counts(log)
    expect(len(n) == 3 and n[1][1] == n[0][1] - 1 and n[1][0] == n[0][0] - 1,
           "Delete on a selected nub removes that nub only (%s)" % (n,))
    m = re.search(r"key viewer selectall: ok focus=\S+ selected=(\d+)", log)
    expect(m is not None and n and int(m.group(1)) == n[-1][0],
           "Ctrl+A with the view focused selects every atom (%s of %s)"
           % (m.group(1) if m else None, n[-1][0] if n else None))
    t = re.search(r"key text selectall: ok focus=\S+ text=\[(.*)\] "
                  r"selected=(\d+)", log)
    expect(t is not None and t.group(2) == "0",
           "Ctrl+A in a text field leaves the structure alone (%s)"
           % ((t.groups() if t else None),))


def check(display, results, timeout=120):
    results.checks += 1
    problems = []

    def expect(ok, what):
        print("%s  builder metal: %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            problems.append(what)

    piCase(display, results, expect)
    cpCase(display, results, expect)
    editCase(display, results, expect)
    if problems:
        results.fail("builder-metal", "; ".join(problems))


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def main():
    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    results = _Results()
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                check(display, results)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  builder metal")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
