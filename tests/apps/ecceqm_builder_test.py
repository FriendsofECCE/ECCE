#!/usr/bin/env python3
"""
The Builder on an ECCE-QM calculation: the orbitals are there and draw, and
the MO correlation diagram is not offered.

The calculation is tests/ecceqm/fixtures/o2-roks, made by
tests/launch/ecceqm_test.py --export from a real run of the bundled engine
(O2 triplet, restricted open-shell B3LYP/6-31G*), in local data mode.  As a
control the same is done with an ORCA calculation (tests/mocomp/fixtures/water)
where the MO Diagram must be offered, so the test would notice if it stopped
looking.

For each calculation the Builder opens with ECCE_OPEN_PANEL=MOs and
ECCE_PANEL_METRICS, whose file lists the panes the window has; "MOs" must be
one of them, "MO Diagram" must be one only for the ORCA calculation.  The MO
panel's message line is read from the log too: ECCE-QM's coefficients are in
the order the stored basis expects, so there is no coefficient-width message
and no "does not have a valid basis set".

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402

QM_CALC = os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks")
ORCA_CALC = os.path.join(REPO, "tests", "mocomp", "fixtures", "water")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
SCREEN = "1366x768x24"

failures = []


def readPanes(path):
    panes = {}
    if os.path.exists(path):
        with open(path) as handle:
            for line in handle:
                m = re.match(r'pane "([^"]+)" (-?\d+) (-?\d+) (\d+) (\d+) (\w+) (\w+)',
                             line)
                if m:
                    panes[m.group(1)] = m.group(6) == "onscreen"
    return panes


def installLocal(folder, calcs):
    """A local data folder with a project holding the given {name: source}."""
    shutil.rmtree(folder, ignore_errors=True)
    home = os.path.join(folder, "users", "local")
    project = os.path.join(home, "qm")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    urls = {}
    for name, source in calcs.items():
        shutil.copytree(source, os.path.join(project, name), symlinks=True)
        urls[name] = "file://" + os.path.join(project, name) + "/"
    return urls


def openBuilder(display, url, label, expectDiagram):
    metrics = tempfile.mktemp(prefix="ecce-ecceqm-", suffix=".txt")
    env = {"ECCE_OPEN_PANEL": "MOs", "ECCE_PANEL_METRICS": metrics}
    result = apps.run(display, "builder", args=("-context", url),
                      windowTimeout=60, settle=40, env=env)
    panes = readPanes(metrics)
    try:
        os.unlink(metrics)
    except OSError:
        pass
    log = result.log or ""
    problem = None
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not result.sawWindow:
        problem = "no window"
    elif not panes:
        problem = "no panel metrics written"
    elif "MOs" not in panes:
        problem = "no MOs panel (%s)" % ", ".join(sorted(panes))
    elif ("MO Diagram" in panes) != expectDiagram:
        problem = ("MO Diagram %s, expected %s (panes: %s)"
                   % ("offered" if "MO Diagram" in panes else "not offered",
                      "offered" if expectDiagram else "not offered",
                      ", ".join(sorted(panes))))
    elif "coefficient" in log.lower() and "width" in log.lower():
        problem = "the MOs panel complained about the coefficient width"
    elif "valid basis set" in log:
        problem = "the MOs panel found no valid basis set"
    if problem is None:
        print("ok    %s" % label)
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 15)))


def main():
    xdisplay.SCREEN = SCREEN

    def checkApp(display, name, results, verbose=False):
        for path in (QM_CALC, ORCA_CALC):
            if not os.path.isdir(path):
                results.notes.append("%s is missing; skipped" % path)
                return
        folder = os.path.join(fixture.stateDir(), "ecceqm-localdata")
        urls = installLocal(folder, {"o2": QM_CALC, "water-orca": ORCA_CALC})
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            openBuilder(display, urls["o2"], "ECCE-QM O2: MOs, no MO Diagram", False)
            openBuilder(display, urls["water-orca"],
                        "control (ORCA water): MOs and MO Diagram", True)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("ecceqm", "%d wrong: %s" % (len(failures), ", ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
