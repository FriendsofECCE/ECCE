#!/usr/bin/env python3
"""Builder: a shape change leaves the nubs at the length of other shapes (#506).

    tests/apps/run_tests.py --check builder_nub_shape_test

Headless, through ECCE_BUILDER_SCRIPT on benzene.car: add an atom, then
change its shape (see CASES) as a click on the atom with another shape
chosen does. Every nub must stay within 10 percent of the tetrahedral nub
length of that element.
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
        #  A private state directory has no saved login: without -pipe the
        #  builder stops at the "ECCE Authentication" dialog.
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth, STRUCTURE),
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


SITE = (8.0, 0.0, 0.0)

#  (element, first shape, new shape, nubs on the new shape).  Every case is a
#  real change: an atom clicked again with its own element and shape gets a
#  new atom bonded to it instead (#401).  C to trigonal bipyramidal is the
#  reported case.
CASES = (("Fe", "Tetrahedral", "TrigonalBipyramidal", 5),
         ("Fe", "Tetrahedral", "Octahedral", 6),
         ("Fe", "Octahedral", "Tetrahedral", 4),
         ("C", "Tetrahedral", "TrigonalBipyramidal", 5))


def nubLengths(atoms, symbol):
    """Distances of every Nub from the SYMBOL atom at SITE (benzene has no
    nubs of its own, so all are that atom's)."""
    centre = min((a for a in atoms if a[0] == symbol),
                 key=lambda a: math.dist(SITE, a[1:]))[1:]
    return [math.dist(centre, a[1:]) for a in atoms if a[0] == "Nub"]


def shapeCase(display, expect):
    got, base = [], {}
    for elem, first, shape, count in CASES:
        name = "%s %s->%s" % (elem, first, shape)
        log = run(display, "%s-%s-%s" % (elem, first, shape), [
            "wait 3000", "add %s %s %g %g %g" % ((elem, first) + SITE),
            "coords", "addelem " + elem, "add %s %s 0 0 0" % (elem, shape),
            "coords", "quit"])
        bl = blocks(log)
        if len(bl) != 2:
            expect(False, "%s: script ran (%d dumps)\n%s"
                   % (name, len(bl), tail(log)))
            continue
        added, changed = nubLengths(bl[0], elem), nubLengths(bl[1], elem)
        if first == "Tetrahedral" and added:
            base[elem] = max(added)
        expect(sum(a[0] == elem for a in bl[1])
               == sum(a[0] == elem for a in bl[0]),
               "%s: changed the atom, added none" % name)
        got.append((name, elem, added, changed, count))
    for elem, length in base.items():
        expect(length > 0.1, "tetrahedral %s has nubs (%.3f A)"
               % (elem, length))
    for name, elem, added, changed, count in got:
        expect(len(changed) == count, "%s: %d nubs after the change (%d)"
               % (name, len(changed), count))
        if elem not in base:
            continue
        for what, lengths in (("added", added), ("changed", changed)):
            worst = max([abs(l - base[elem]) / base[elem] for l in lengths]
                        or [1.0])
            expect(worst <= 0.10, "%s %s: every nub within 10%% of the "
                   "tetrahedral length %.3f A (worst %.0f%%)"
                   % (name, what, base[elem], 100 * worst))


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
