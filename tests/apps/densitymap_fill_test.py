#!/usr/bin/env python3
"""
Density Map "Fill" tick (#1077), headless: dmfill on|off with dmpng.

  an orbital with both signs: blue and orange areas appear with Fill, not
  without; a pixel just inside the frame, far from the molecule, stays white
  a total density gets viridis colours with Fill and none without
  Fill defaults to off and is reported by dmstate

    ECCE_DMFILL_PNG=<dir>   also copy the pictures there

Same installed tree and isolation as densityprofile_pick_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
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

EX = os.path.join(REPO, "data", "client", "examples", "genchem",
                  "molecular-orbitals-of-diatomics", "ecceqm-HF", "stored")
HF = os.path.join(EX, [x for x in os.listdir(EX) if os.path.isdir(os.path.join(EX, x))][0])

SCENE = """densitymap {t}-pi-off 800 600 4 alpha 0
dmstate
dmfill on
dmpng {t}-pi-on 800 600
dmstate
densitymap {t}-tot-on 800 600 0 alpha 0
dmfill off
dmpng {t}-tot-off 800 600
dmstate
hold 1
"""


def counts(path):
    from PIL import Image
    import numpy as np
    im = np.asarray(Image.open(path).convert("RGB")).astype(int)
    r, g, b = im[..., 0], im[..., 1], im[..., 2]
    #  Frame is x 62.., y 34..; the legend colour bar is right of it, so look at the plot only.
    r, g, b = r[34:540, 62:560], g[34:540, 62:560], b[34:540, 62:560]
    blue = ((b - r > 60) & (b > 120)).sum()
    orange = ((r - b > 80) & (r > 150) & (g > 60)).sum()
    viridis = ((abs(r - 33) < 45) & (abs(g - 145) < 45) & (abs(b - 140) < 45)).sum()
    return int(blue), int(orange), int(viridis), tuple(im[38, 66])


def check(out, t, log):
    p = []
    fills = [l.split()[1] for l in log.splitlines() if l.startswith("fill ")]
    runs = [f for i, f in enumerate(fills) if i == 0 or f != fills[i - 1]]   # every dm command prints the state
    if runs != ["0", "1", "0"]:
        p.append("dmstate fill values %s, wanted 0 1 0" % runs)
    off = counts(os.path.join(out, t + "-pi-off.png"))
    on = counts(os.path.join(out, t + "-pi-on.png"))
    if on[0] < 3000 or on[1] < 3000:
        p.append("orbital with Fill: blue %d orange %d pixels" % on[:2])
    if off[0] > 1500 or off[1] > 1500:
        p.append("orbital without Fill: blue %d orange %d pixels" % off[:2])
    if on[3] != (255, 255, 255):
        p.append("far from the molecule is %s, not white" % (on[3],))
    tot_on = counts(os.path.join(out, t + "-tot-on.png"))
    tot_off = counts(os.path.join(out, t + "-tot-off.png"))
    if tot_on[2] < 3000 or tot_off[2] > 3000:
        p.append("total density viridis pixels: fill %d, no fill %d" % (tot_on[2], tot_off[2]))
    if tot_on[3] != (255, 255, 255):
        p.append("total density far corner is %s, not white" % (tot_on[3],))
    return p


def main():
    pngdir = os.environ.get("ECCE_DMFILL_PNG")

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "dmfill-localdata")
        urls = installLocal(folder, {"hf": HF})
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            t = "dmf"
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE.format(t=t))
            result = apps.run(display, "builder", args=("-context", urls["hf"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_PANEL_FULLSCREEN": "1",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            problems = check(out, t, result.log or "")
            if pngdir:
                os.makedirs(pngdir, exist_ok=True)
                for png in os.listdir(out):
                    if png.startswith(t):
                        shutil.copy(os.path.join(out, png), pngdir)
            print("%s  densitymap fill%s" % ("ok  " if not problems else "FAIL",
                                             "" if not problems else ": " + "; ".join(problems)))
            if problems:
                results.fail("densitymap_fill", "; ".join(problems))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
