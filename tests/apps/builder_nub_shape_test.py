#!/usr/bin/env python3
"""Builder: a shape change leaves the nubs at the length of other shapes (#506).

    tests/apps/builder_nub_shape_test.py        own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on benzene.car: add an Fe atom, then
change its shape to trigonal bipyramidal, octahedral and (as the baseline)
tetrahedral, as a click on the atom with another shape chosen does. Every
nub must stay within 10 percent of the tetrahedral nub length.
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
    work = os.path.join(fixture.stateHome(), "builder-nubshape-" + name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "nubshape.script")
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


def nubLengths(atoms, symbol):
    """Distances of every Nub from the last SYMBOL atom (benzene has none
    of its own, so all are the metal's)."""
    centre = [a for a in atoms if a[0] == symbol][-1][1:]
    return [math.dist(centre, a[1:]) for a in atoms if a[0] == "Nub"]


def shapeCase(display, expect):
    got = {}
    for shape, count in (("Tetrahedral", 4), ("TrigonalBipyramidal", 5),
                         ("Octahedral", 6)):
        log = run(display, shape, [
            "wait 3000", "add Fe Tetrahedral 8 0 0", "coords",
            "selectelem Fe", "add Fe %s 0 0 0" % shape, "coords", "quit"])
        bl = blocks(log)
        if len(bl) != 2:
            expect(False, "%s: script ran (%d dumps)\n%s"
                   % (shape, len(bl), tail(log)))
            continue
        got[shape] = (nubLengths(bl[0], "Fe"), nubLengths(bl[1], "Fe"), count)
    if "Tetrahedral" not in got:
        return
    base = max(got["Tetrahedral"][0] or [0])
    expect(base > 0.1, "tetrahedral Fe has nubs (%.3f A)" % base)
    for shape, (added, changed, count) in got.items():
        expect(len(changed) == count, "%s: %d nubs after the change (%d)"
               % (shape, len(changed), count))
        for what, lengths in (("added", added), ("changed", changed)):
            worst = max([abs(l - base) / base for l in lengths] or [1.0])
            expect(worst <= 0.10, "%s %s: every nub within 10%% of the "
                   "tetrahedral length %.3f A (worst %.0f%%)"
                   % (shape, what, base, 100 * worst))


def check(display, results, timeout=120):
    results.checks += 1
    problems = []

    def expect(ok, what):
        print("%s  builder nub shape: %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            problems.append(what)

    shapeCase(display, expect)
    if problems:
        results.fail("builder-nub-shape", "; ".join(problems))


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
        print("PASS  builder nub shape")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
