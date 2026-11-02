#!/usr/bin/env python3
"""Builder: Edit > Distort Bonds on an octahedral Fe, undone (#619).

    tests/apps/builder_distort_test.py          own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT: "cmd distort" is the command the
dialog runs.  Every Fe-nub distance changes by the given amount, the Jahn-
Teller preset gives +d on one trans pair and -d/2 on the other four, and
Ctrl+Z restores the coordinates exactly.
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
    work = os.path.join(fixture.stateHome(), "builder-distort-" + name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "distort.script")
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



def dumps(log):
    """Each coordsraw as a list of (x, y, z)."""
    out = []
    for m in re.finditer(r"BUILDER: rawatom (\d+) (\S+) (\S+) (\S+)", log):
        if m.group(1) == "1":
            out.append([])
        out[-1].append(tuple(float(m.group(k)) for k in (2, 3, 4)))
    return out


def dist(a, b):
    return math.sqrt(sum((p - q) ** 2 for p, q in zip(a, b)))


def check(display, results, timeout=120):
    results.checks += 1
    problems = []

    def expect(ok, what):
        print("%s  builder distort: %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            problems.append(what)

    log = run(display, "all", [
        "wait 3000", "add Fe Octahedral 8 0 0", "coordsraw",
        "cmd distort centre 13 dr 0.1", "coordsraw",
        "key viewer undo", "coordsraw",
        "cmd distort centre 13 jt 0.1 axis longest", "coordsraw", "quit"])
    d = dumps(log)
    expect(len(d) == 4, "four coordinate dumps (%d)\n%s" % (len(d), tail(log)))
    if len(d) == 4:
        before, after, undone, jt = d
        fe = 12
        nubs = list(range(13, len(before)))
        expect(len(nubs) == 6, "Fe has six nubs (%d)" % len(nubs))
        steps = [dist(after[i], after[fe]) - dist(before[i], before[fe])
                 for i in nubs]
        expect(all(abs(s - 0.1) < 1e-6 for s in steps),
               "every Fe-X distance grew by 0.1 (%s)" % steps)
        expect(dist(after[fe], before[fe]) < 1e-9, "the centre stayed")
        dirs = []
        for i in nubs:
            v0 = [a - b for a, b in zip(before[i], before[fe])]
            v1 = [a - b for a, b in zip(after[i], after[fe])]
            n0, n1 = math.sqrt(sum(x * x for x in v0)), math.sqrt(sum(x * x for x in v1))
            dirs.append(max(abs(a / n0 - b / n1) for a, b in zip(v0, v1)))
        expect(max(dirs) < 1e-6, "bond directions unchanged")
        expect(all(dist(a, b) < 1e-9 for a, b in zip(before, undone)),
               "undo restores every coordinate")
        j = sorted(round(dist(jt[i], jt[fe]) - dist(before[i], before[fe]), 6)
                   for i in nubs)
        expect(j == [-0.05] * 4 + [0.1] * 2,
               "Jahn-Teller: two +0.1, four -0.05 (%s)" % j)
    if problems:
        results.fail("builder-distort", "; ".join(problems))
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
