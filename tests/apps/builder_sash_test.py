#!/usr/bin/env python3
"""Builder column: the sashes above and below Build move Build's edges, and
no sash drag leaves the frame holding the mouse.

    tests/apps/builder_sash_test.py             own Xvfb and services

The Builder's ECCE_VIEWER_SCENE hook "sashdrag" presses on the sash under a
pane, moves in ten steps and releases, through the frame's event chain as
the mouse does (wxAUI first); "lost" drops the release.  Atom Table is
folded first so the column has room above the panes' floors (a column at
its floors cannot move any sash).  The last drag has no resizable pane
below its sash, the case in which wxAUI ended the drag inside a motion and
captured the mouse again.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                 # noqa: E402
import fixture              # noqa: E402
import pbc_ux_test          # noqa: E402
from builder_panes_test import run, rect, lines, _Results   # noqa: E402

TOLERANCE = 12
TOOLS = "Open_structures Build Selection Atom_Table Symmetry"


def check(display, results):
    url, error = pbc_ux_test.install("sash-unrun", pbc_ux_test.notRun)
    if error:
        results.fail("builder sash", "cannot install fixture: %s" % error)
        return
    results.checks += 1
    work = run(display, results, "sash", url,
               ["columntab 0", "panefold Atom_Table",
                "panestate s0 " + TOOLS,
                "sashdrag Build -50", "panestate s1 " + TOOLS,
                "sashdrag Build 30", "panestate s2 " + TOOLS,
                "sashdrag Open_structures 40", "panestate s3 " + TOOLS,
                "sashdrag Build -20 lost", "panestate s4 " + TOOLS,
                "panefold Symmetry", "sashdrag Selection 30"], "detail")

    def h(state, pane):
        r = rect(work, state, pane)
        return r[1] if r else None

    problems = []
    drags = lines(work, "sash")
    for line in drags:
        if line.endswith("capture 1"):
            problems.append("mouse still captured after: sashdrag %s"
                            % line.rsplit(" capture", 1)[0])
    if len(drags) < 5:
        problems.append("%d of 5 drags reported" % len(drags))
    b = [h("s%d" % i, "Build") for i in range(5)]
    sel = [h("s%d" % i, "Selection") for i in range(5)]
    op = [h("s%d" % i, "Open structures") for i in range(5)]
    print("builder sash: Build %s, Selection %s, Open structures %s"
          % (b, sel, op))
    if None in b + sel + op:
        problems.append("panes missing from the state files")
    else:
        def moved(what, got, want):
            if abs(got - want) > TOLERANCE:
                problems.append("%s: %+d px, expected %+d" % (what, got, want))
        moved("below Build -50: Build", b[1] - b[0], -50)
        moved("below Build -50: Selection", sel[1] - sel[0], 50)
        moved("below Build +30: Build", b[2] - b[1], 30)
        moved("above Build +40: Open structures", op[3] - op[2], 40)
        moved("above Build +40: Build", b[3] - b[2], -40)
        moved("below Build -20, release lost: Build", b[4] - b[3], -20)
    for p in problems:
        results.fail("builder sash", p)


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
    #  Tall enough that the column has room above the panes' floors.
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
        print("PASS  builder sash")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
