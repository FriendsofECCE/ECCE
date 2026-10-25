#!/usr/bin/env python3
"""
The Radial Distribution panel on a stored ECCE-QM neon atom, headless:
Builder scene commands (rd*, plotshot), no input events.  Checked: the
default selection is the occupied orbitals, the 2s curve has two maxima and
the 2p curve one, the total density integrates to 10, the csv and png
exports are written, the docked pane keeps a plot of usable size on a
1366x768 and a 1700x1000 screen.

    ECCE_RDF_SCREENS=1366x768x24,1700x1000x24   screens to try
    ECCE_RDF_PNG=<dir>   also copy the pictures there
    ECCE_RDF_REPORT=1    print the numbers but do not fail

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALCS = {"ne": os.path.join(REPO, "tests", "builder", "radialdist-fixtures", "ne")}
SCREENS = os.environ.get("ECCE_RDF_SCREENS", "1366x768x24,1700x1000x24").split(",")

#  Default state first, then the 1s/2s/2p overlay with the total per
#  electron, the exports, and the docked picture last (it leaves the pane up).
SCENE = """rdopen
rdstate
rdmo 1,2,3
rdtotal per
rdstate
rdcsv {t}-rd
rdunit bohr
rdunit A
rdpng {t}-rd-plot 900 600
plotshot {t}-rd 0 0 Radial Distribution:docked
hold 1
"""


def curves(log, after):
    """The 'curve' lines of the RADIALDIST block that follows `after`."""
    at = log.find(after)
    out = []
    if at < 0:
        return out
    for line in log[at:].splitlines()[1:]:
        if line.startswith("RADIALDIST"):
            break
        m = re.match(r"curve '(.*)' integral ([-\d.e+]+) maxima (\d+)", line)
        if m:
            out.append((m.group(1), float(m.group(2)), int(m.group(3))))
    return out


def main():
    pngdir = os.environ.get("ECCE_RDF_PNG")
    report = bool(os.environ.get("ECCE_RDF_REPORT"))

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "rdf-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            t = "rdf-%s" % xdisplay.SCREEN.split("x")[1]
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE.format(t=t))
            result = apps.run(display, "builder", args=("-context", urls["ne"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_PANEL_FULLSCREEN": "1",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            log = result.log or ""
            problems = []
            first = curves(log, "RADIALDIST: rdopen")
            final = curves(log, "RADIALDIST: rdtotal")
            print("%s default: %s" % (t, first))
            print("%s overlay: %s" % (t, final))
            if len(first) != 5:
                problems.append("default selection is %d curves, not the 5 occupied" % len(first))
            if len(final) != 4:
                problems.append("overlay has %d curves, not 4" % len(final))
            else:
                if final[1][2] != 2:
                    problems.append("2s has %d maxima, not 2" % final[1][2])
                if final[2][2] != 1:
                    problems.append("2p has %d maxima, not 1" % final[2][2])
                if abs(final[3][1] - 1.0) > 2e-3:
                    problems.append("total per electron integrates to %.4f" % final[3][1])
            csv = os.path.join(out, t + "-rd.csv")
            if not os.path.exists(csv):
                problems.append("no csv")
            else:
                head = open(csv).read().splitlines()[:2]
                if not head[1].startswith("r,") or "Total density" not in head[1]:
                    problems.append("csv header %r" % head)
            for png in (t + "-rd.png", t + "-rd-plot.png"):
                if not os.path.exists(os.path.join(out, png)):
                    problems.append("no " + png)
                elif pngdir:
                    os.makedirs(pngdir, exist_ok=True)
                    shutil.copy(os.path.join(out, png), pngdir)
            for line in log.splitlines():
                if line.startswith("PLOTSHOT-PLOT") or line.startswith("PLOTSHOT-CURVE") \
                        or (line.startswith("PLOTSHOT: ") and "window" in line):
                    print("   " + line)
            m = re.search(r"PLOTSHOT: %s-rd (\d+)x(\d+)" % t, log)
            if not m:
                problems.append("no docked picture")
            elif int(m.group(1)) < 300 or int(m.group(2)) < 300:
                problems.append("docked pane %sx%s" % m.groups())
            print("%s  %s%s" % ("ok  " if not problems else "FAIL", t,
                                "" if not problems else ": " + "; ".join(problems)))
            bad += problems
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad and not report:
            results.fail("radialdist", "; ".join(bad))

    run_tests.checkApp = checkApp
    for screen in SCREENS:
        xdisplay.SCREEN = screen
        argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
        sys.argv = argv[:3] + [a for a in argv[3:] if a not in ("--app", "builder")]
        rc = run_tests.main()
        if rc:
            return rc
    return 0


if __name__ == "__main__":
    sys.exit(main())
