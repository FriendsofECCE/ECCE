#!/usr/bin/env python3
"""
The docked MOs and Density Map panes at their default size (#510, #533):
the open panel takes its height from the Properties list above it, so the
MOs table shows rows and the Density Map's plot keeps room, on a 1366x768
and a 1700x1000 screen.  Headless: Builder scene commands, no input events.

    ECCE_DOCKED_SCREENS=1366x768x24,1700x1000x24   screens to try
    ECCE_DOCKED_PNG=<dir>   also copy the pictures there
    ECCE_DOCKED_TAG=<tag>   picture name prefix (default "after")
    ECCE_DOCKED_REPORT=1    print the numbers but do not fail

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

EXAMPLES = os.path.join(REPO, "data", "client", "examples", "organic")
CALCS = {"ethene": os.path.join(EXAMPLES, "ecceqm-ethene", "stored", "ethene")}
SCREENS = os.environ.get("ECCE_DOCKED_SCREENS",
                         "1366x768x24,1700x1000x24").split(",")

#  Total density first, then an MO computed in the MOs panel, as in #510;
#  then the HOMO (zero in the molecular plane), whose levels are scaled (#532).
#  One session each: a docked pane left by one shot shows through the next
#  one's picture.
SCENES = ["plotshot {t}-mos 0 0 MOs:dockedtable\nhold 1\n",
          """plotshot {t}-dm-total 0 0 Density Map:docked
dmfollow 7
plotshot {t}-dm-mo7 0 0 Density Map:docked
dmstate
densitymap {t}-dm-node 700 560 8 alpha 0
dmoffset 1.8
dmpng {t}-dm-weak 700 560
hold 1
"""]


def main():
    pngdir = os.environ.get("ECCE_DOCKED_PNG")
    report = bool(os.environ.get("ECCE_DOCKED_REPORT"))
    tag = os.environ.get("ECCE_DOCKED_TAG", "after")

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "docked-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            for url in urls.values():
                t = "%s-%s" % (tag, xdisplay.SCREEN.split("x")[1])
                out = os.path.join(folder, "shots")
                os.makedirs(out, exist_ok=True)
                scene = os.path.join(folder, "scene")
                log = ""
                for text in SCENES:
                    with open(scene, "w") as h:
                        h.write(text.format(t=t))
                    result = apps.run(display, "builder", args=("-context", url),
                                      windowTimeout=120, settle=300,
                                      env={"ECCE_PANEL_FULLSCREEN": "1",
                                           "ECCE_VIEWER_SCENE": scene,
                                           "ECCE_VIEWER_SCENE_OUT": out})
                    log += result.log or ""
                problems = []
                mo = log.find("PLOTSHOT-MOTABLE %s-mos" % t)
                grid = re.search(r"^gridRect (\d+) (\d+)", log[mo:], re.M) if mo >= 0 else None
                height = int(grid.group(2)) if grid else 0
                print("%s: MOs table %d px high" % (t, height))
                if height < 20 * 4 + 25:
                    problems.append("MOs table only %d px high" % height)
                for m in re.finditer(r"^canvas (\d+) (\d+)", log, re.M):
                    w, h = int(m.group(1)), int(m.group(2))
                    print("%s: Density Map canvas %dx%d" % (t, w, h))
                    if w < 200 or h < 150:
                        problems.append("Density Map canvas %dx%d" % (w, h))
                for line in log.splitlines():
                    if (line.startswith("PLOTSHOT-PANE") or (line.startswith("PLOTSHOT: ") and "window" in line)):
                        print("   " + line)
                for png in os.listdir(out):
                    if png.startswith(t) and pngdir:
                        os.makedirs(pngdir, exist_ok=True)
                        shutil.copy(os.path.join(out, png), pngdir)
                print("%s  %s%s" % ("ok  " if not problems else "FAIL", t,
                                    "" if not problems else ": " + "; ".join(problems)))
                bad += problems
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad and not report:
            results.fail("docked", "; ".join(bad))

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
