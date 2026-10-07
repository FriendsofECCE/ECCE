#!/usr/bin/env python3
"""
Builder > Tools > Dock Floating Panels (#53), without a drag.

The Builder's ECCE_TEST_DOCK hook floats every panel docked on the left, so
that no left dock is left, runs the real menu handler and prints how many
panels were floating and docked on the left before and after.  The command
must return every one of them to that side, which has to recreate the dock
that emptied.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

failures = []


def checkDock(display):
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    os.environ["ECCE_TEST_DOCK"] = "1"
    try:
        result = apps.run(display, "builder", args=("-pipe", authPath),
                          windowTimeout=60, settle=60)
    finally:
        os.environ.pop("ECCE_TEST_DOCK", None)
    log = result.log or ""
    m = re.search(r"ECCE_TEST_DOCK: moved (\d+), floating (\d+), left docked "
                  r"(\d+); after: floating (\d+), left docked (\d+), all back "
                  r"(\d)", log)
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not m:
        problem = "hook never reported"
    else:
        moved, fl, left, fl2, left2, back = map(int, m.groups())
        if moved < 2:
            problem = "only %d panels were docked on the busiest side" % moved
        elif left != 0 or fl != moved:
            problem = ("before: %d floating, %d still docked on that side; "
                       "expected %d and 0" % (fl, left, moved))
        elif fl2 != 0 or left2 != moved or not back:
            problem = ("after Dock Floating Panels: %d floating, %d docked "
                       "on the left, all back=%d; expected 0, %d, 1"
                       % (fl2, left2, back, moved))
        else:
            problem = None
            print("ok    Dock Floating Panels returned %d panels to an "
                  "emptied dock" % moved)
    if problem:
        failures.append(problem)
        print("FAIL  Dock Floating Panels: %s\n%s"
              % (problem, run_tests._tail(log, 25)))


def main():
    def checkApp(display, name, results, verbose=False):
        checkDock(display)
        if failures:
            results.fail("dock", "Dock Floating Panels wrong: "
                         + "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
