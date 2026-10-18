#!/usr/bin/env python3
"""Builder: the Log pane at the bottom starts folded, unfolds, folds back to its
caption, and is the frame's full width each time (#347).

    tests/apps/builder_log_fold_test.py         own Xvfb and services

The fold button's click ("foldclick") goes through the frame and manager as
the mouse does.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                 # noqa: E402
import fixture              # noqa: E402
import pbc_ux_test          # noqa: E402
from builder_panes_test import run, rect, flag, shownWidth, _Results   # noqa: E402


def check(display, results):
    url, error = pbc_ux_test.install("logfold-unrun", pbc_ux_test.notRun)
    if error:
        results.fail("builder log fold", "cannot install fixture: %s" % error)
        return
    results.checks += 1
    work = run(display, results, "logfold", url,
               ["panestate l0 Log", "foldclick Log", "panestate l1 Log",
                "foldclick Log", "panestate l2 Log"], "detail")
    problems = []
    r = [rect(work, "l%d" % i, "Log") for i in range(3)]
    f = [flag(work, "l%d" % i, "Log", "folded") for i in range(3)]
    #  The Log spans the Viewer's column, as wide as the Viewer above it.
    client = shownWidth(work, "l0", "Viewer") or 0
    if None in r or not client:
        problems.append("no Log state: %s" % r)
    else:
        if f != [1, 0, 1]:
            problems.append("Log folded flags %s, expected [1, 0, 1]" % f)
        for i in range(3):
            if r[i][0] < client:
                problems.append("Log width %d of the Viewer's %d at step %d (half width?)"
                                % (r[i][0], client, i))
        if r[1][1] <= r[0][1] + 20:
            problems.append("Log did not grow when unfolded: %s" % r)
        if r[2][1] > r[0][1] + 10:
            problems.append("Log stays tall after folding: %s" % r)
    for p in problems:
        results.fail("builder log fold", p)


def main():
    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    if not apps.installed("builder"):
        print("SKIP  no installed builder")
        return 77
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    results = _Results()
    #  A tall screen, as the other pane tests.
    xdisplay.SCREEN = "1920x1200x24"
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
        print("PASS  builder log fold")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
