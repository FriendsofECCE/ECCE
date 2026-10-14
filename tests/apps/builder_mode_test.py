#!/usr/bin/env python3
"""Builder: Structure Library mode stays on after an insert, says what it is, and
the toolbar always shows the mode in effect (#321 item 9).

    tests/apps/builder_mode_test.py             own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on benzene.car.  "info" reports the
mode in effect, the mode the toolbar shows and whether the viewer picks;
"key escape" is what the Esc hook does with the 3-D view focused.
"""
import os
import math
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(ROOT, "tests", "fragreaders", "data", "benzene.car")

SCRIPT = [
    "wait 3000", "info",
    "library Teaching/Diatomics CO", "info",
    # a hydrogen picked against a library atom with no open site: no connect
    "selectelem H", "libatom 0", "click", "info",
    "key escape", "info",
    # free-space inserts stay in the mode and never land on atoms
    "cmd clear",
    "select -1", "library SimpleStructures/Simple_arenes Benzene",
    "libatom -1", "click", "info",
    "select -1", "click", "info", "coords",
    "key escape", "info", "quit",
]


def tail(log, n=25):
    return "\n".join("      | " + l for l in log.splitlines()[-n:])


def check(display, results, timeout=120):
    work = os.path.join(fixture.stateHome(), "builder-mode")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    results.checks += 1
    try:
        path = os.path.join(work, "mode.script")
        with open(path, "w") as handle:
            handle.write("\n".join(SCRIPT) + "\n")
        result = apps.run(display, "builder", args=(STRUCTURE,),
                          windowTimeout=60, settle=timeout,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1",
                               "ECCE_TEST_XYZ_UNITS": "angstrom"})
        log = result.log or ""
        modes = re.findall(r"BUILDER: mode=(\w+) toolbar=(\w+) checked=(\d+) "
                           r"viewer=(\w+)", log)
        atoms = [(int(a), int(n)) for a, n in re.findall(
            r"BUILDER: atoms=(\d+) lattice=\w+ nubs=(\d+)", log)]
        if len(modes) != 7 or len(atoms) != 7:
            results.fail("builder-mode", "%d mode lines and %d atom lines, "
                         "expected 7 each\n%s" % (len(modes), len(atoms),
                                                  tail(log)))
            return
        problems = []

        def expect(ok, what):
            print("%s  builder mode: %s" % ("ok  " if ok else "FAIL", what))
            if not ok:
                problems.append(what)

        for i, (mode, toolbar, checked, viewer) in enumerate(modes):
            expect(mode == toolbar and checked == "1"
                   and (viewer == "picking") == (mode != "other"),
                   "step %d: mode %s, toolbar %s (%s checked), viewer %s agree"
                   % (i, mode, toolbar, checked, viewer))
        expect(modes[0][0] == "select", "starts in Select")
        expect(modes[1][0] == "structlib", "the Structure Library pane "
               "enters Structure Library mode")
        expect("Structure Library: click an open site to attach, or press "
               "Esc for Select" in log, "the message line names the mode")
        expect(modes[2][0] == "structlib", "a failed connect stays in the "
               "mode")
        expect("Cannot unambiguously connect fragments. Select one open "
               "site on each part, or press Esc." in log,
               "the failed connect says what to do")
        expect(atoms[2] == atoms[1], "a failed connect adds no atom and turns "
               "no hydrogen into a Nub (%s -> %s atoms, nubs)"
               % (atoms[1], atoms[2]))
        expect(modes[3][0] == "select", "Esc returns to Select")
        expect(modes[4][0] == "structlib" and modes[5][0] == "structlib",
               "inserts from the Structure Library keep the mode")
        expect(atoms[4][0] == 12 and atoms[5][0] == 24,
               "two free-space inserts of benzene give 24 atoms (%s, %s)"
               % (atoms[4][0], atoms[5][0]))
        xyz = [tuple(float(v) for v in m) for m in re.findall(
            r"BUILDER: atom \d+ \w+ (\S+) (\S+) (\S+)", log)]
        closest = min((math.dist(p, q) for i, p in enumerate(xyz)
                       for q in xyz[i + 1:]), default=0)
        expect(len(xyz) == 24 and closest >= 0.5,
               "no two atoms closer than 0.5 A (%d atoms, closest %.2f A)"
               % (len(xyz), closest))
        expect(modes[6][0] == "select", "Esc leaves Structure Library mode")
        if problems:
            results.fail("builder-mode", "; ".join(problems) + "\n" + tail(log))
    finally:
        shutil.rmtree(work, ignore_errors=True)


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
        print("PASS  builder mode")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
