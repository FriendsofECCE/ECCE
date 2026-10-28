#!/usr/bin/env python3
"""
Builder layout hooks, headless: the MO Diagram's Model MOs labels do not
overlap (HCN), the Density Map zooms to a window at the same contour levels
and resets (ethene), and the right-hand column can be dragged to 70 % of the
window with the viewer keeping a minimum width.  Scene commands only.

    ECCE_LAYOUT2_PNG=<dir>   also copy the pictures there
    ECCE_LAYOUT2_SCREEN=1366x768x24

Same installed tree and isolation as docked_panels_test.py.  Exit 77 (skip)
without an install or Xvfb.
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

EXAMPLES = os.path.join(REPO, "data", "client", "examples", "organic", "bonding-in-hydrocarbons")
CALCS = {
    "ethene": os.path.join(EXAMPLES, "ecceqm-ethene", "stored", "ethene"),
    "hcn": os.path.join(REPO, "tests", "builder", "radialdist-fixtures", "hcn"),
}
SCENES = {
    "hcn": "plotshot {t}-modiagram 0 0 MO Diagram:docked\nhold 1\n",
    "hcn-short": "plotshot {t}-modiagram-short 1000 330 MO Diagram\nplotshot {t}-modiagram-tiny 1000 250 MO Diagram\nhold 1\n",
    "ethene": """densitymap {t}-dm-whole 700 560 0 alpha 0
dmstate
dmzoom 1.25 @1
dmzoom 2 @1
dmzoom 2 @1
dmpng {t}-dm-zoom 700 560
dmstate
dmzoomreset
dmstate
dmwheel 2 @1
dmrefine
dmzoomreset
hold 1
""",
    "ethene-wide": """list Density Map
hold 2
columnwidth 700
panestate wide Viewer
columnwidth 1000
panestate wider Viewer
plotshot {t}-wide 0 0 Density Map:docked
hold 1
""",
}


def main():
    pngdir = os.environ.get("ECCE_LAYOUT2_PNG")
    screen = os.environ.get("ECCE_LAYOUT2_SCREEN", "1366x768x24")
    tag = "l2"

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "layout2-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        problems = []
        try:
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            for key, text in SCENES.items():
                url = urls[key.split("-")[0]]
                scene = os.path.join(folder, "scene")
                with open(scene, "w") as h:
                    h.write(text.format(t=tag))
                res = apps.run(display, "builder", args=("-context", url),
                               windowTimeout=120, settle=300,
                               env={"ECCE_PANEL_FULLSCREEN": "1",
                                    "ECCE_VIEWER_SCENE": scene,
                                    "ECCE_VIEWER_SCENE_OUT": out})
                log = res.log or ""
                print("--- " + key)
                if key == "ethene":
                    wins = re.findall(r"^zoom (\d) window (\S+) (\S+) (\S+) (\S+)", log, re.M)
                    print("   windows:", wins)
                    #  every command reports the state: first whole, then
                    #  narrower while zoomed, whole again after the reset.
                    width = [float(x[2]) - float(x[1]) for x in wins]
                    if not wins or wins[0][0] != "0" or wins[-1][0] != "0":
                        problems.append("zoom state sequence %s" % wins)
                    elif not min(width) < width[0] / 3:
                        problems.append("zoomed window %.2f of %.2f" % (min(width), width[0]))
                    #  the wheel draws a coarse grid at once; dmrefine (the
                    #  panel's rest timer) the full one, with the zoom's step.
                    steps = {c: float(g) for c, g in re.findall(
                        r"^DENSITYMAP: (dmwheel|dmrefine)\n(?:.*\n)*?grid \d+ \d+ (\S+)", log, re.M)}
                    print("   wheel steps:", steps)
                    if len(steps) != 2 or not steps["dmwheel"] > steps["dmrefine"]:
                        problems.append("wheel zoom steps %s" % steps)
                if key == "ethene-wide":
                    for n in ("wide", "wider"):
                        p = os.path.join(out, n + ".txt")
                        print(open(p).read() if os.path.exists(p) else "no " + n)
                    sash = os.path.join(out, "sash.txt")
                    got = open(sash).read() if os.path.exists(sash) else ""
                    print(got)
                    m = re.findall(r"columnwidth (\d+) got (\d+) viewer (\d+)", got)
                    if len(m) < 2:
                        problems.append("no column width report")
                    else:
                        want, g, v = map(int, m[0])
                        if g < want * 0.95 or v < 200:
                            problems.append("column %d wanted %d, viewer %d" % (g, want, v))
                for line in log.splitlines():
                    if line.startswith("PLOTSHOT-PANE"):
                        print("   " + line)
                for png in os.listdir(out):
                    if png.startswith(tag) and pngdir:
                        os.makedirs(pngdir, exist_ok=True)
                        shutil.copy(os.path.join(out, png), pngdir)
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        print("%s" % ("ok" if not problems else "FAIL " + "; ".join(problems)))
        if problems:
            results.fail("layout2", "; ".join(problems))

    run_tests.checkApp = checkApp
    xdisplay.SCREEN = screen
    argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    sys.argv = argv[:3] + [a for a in argv[3:] if a not in ("--app", "builder")]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
