#!/usr/bin/env python3
"""
'Use symmetry' goes off by itself for a transition-state search and for a
structure with used constraints or a scan (#507), once per change of state:
a tick by the user stands until the run type or constraint state changes.
The CalcEd script command "symmetry [0|1]" reports (or clicks) the box.

Same installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
"""

import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402
import calced_constraints_line_test as cl  # noqa: E402

LINE = re.compile(r"CALCED: symmetry box=(\d) stored=(\d) note=(.*)")
FIX = os.path.join(HERE, "fixtures")
WHY = "Symmetry off: a transition-state search / scan must be able to break"

# label, fixture, constraints?, [(step, box expected afterwards)]; the
# fixtures open on a run type that ignores the constraints
CASES = [
    ("ECCE-QM-TS", os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geometry"), False, [
        (None, 1), ("runtype Transition State (Opt-TS)", 0), ("symmetry 1", 1),
        ("runtype Energy", 1), ("runtype Transition State (Opt-TS)", 0)]),
    ("G16-scan", os.path.join(FIX, "calc-polar-h2o-g16"), True, [
        (None, 1), ("runtype Geometry", 0), ("symmetry 1", 1),
        ("runtype Energy", 1), ("runtype Geometry", 0)]),
    ("ORCA-scan", os.path.join(FIX, "calc-orca-water-ccpvdz"), True, [
        (None, 1), ("runtype Geometry", 0), ("runtype Energy", 0)]),
]


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "nosym-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(cl.PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            for label, source, constrained, steps in CASES:
                calc = cl.makeCalc(project, label, source)
                if not constrained:
                    os.remove(os.path.join(calc, "Parameters", "GeomConstraintModel.gcst"))
                script = ["wait 9000", "symmetry"]
                for step, _ in steps[1:]:
                    script += [step, "wait 3000"]
                    if not step.startswith("symmetry"):
                        script.append("symmetry")
                script.append("quit")
                log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", script,
                                   "file://" + calc + "/")
                got = LINE.findall(log)
                sym.check(len(got) == len(steps), "%s: %d reports (got %d)"
                          % (label, len(steps), len(got)), run_tests._tail(log, 30))
                for (step, want), (box, stored, note) in zip(steps, got):
                    sym.check(int(box) == want and int(stored) == want,
                              "%s after %s: box %d (got %s/%s)"
                              % (label, step or "open", want, box, stored))
                    sym.check((WHY in note) == (want == 0 and step != "runtype Energy"),
                              "%s after %s: the note %s" % (label, step or "open",
                              "says why" if want == 0 and step != "runtype Energy"
                              else "is absent"))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-nosym", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
