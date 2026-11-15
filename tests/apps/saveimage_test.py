#!/usr/bin/env python3
"""
Print-quality image export (#1059): the water HOMO saved with the scene hook
"saveimage <png> <factor> white" in the Builder.  The 2x image must be twice
the view's size, show the lobe in a colour blended with the white background
(not the pure lobe colour), and have no screen-door grid inside the lobe.

Same installed tree and isolation as shot_hooks_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without them.
SAVEIMAGE_KEEP=<dir> keeps the PNGs.
"""

import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALCS = {"water": os.path.join(HERE, "fixtures", "calc-water-opt")}
SCENE = """windowsize 900 700
background 1 1 1
viewall
moselect 5
mopanel compute
hold 2
saveimage {out}/x1.png 1 white
saveimage {out}/x2.png 2 white
"""


def check(out, log):
    from PIL import Image
    p = []
    for n in ("x1", "x2"):
        if not os.path.exists(os.path.join(out, n + ".png")):
            return ["%s.png was not written" % n]
    a = Image.open(os.path.join(out, "x1.png")).convert("RGB")
    b = Image.open(os.path.join(out, "x2.png")).convert("RGB")
    if b.size != (2 * a.size[0], 2 * a.size[1]):
        p.append("2x image is %s for a %s view" % (b.size, a.size))
    px = b.load()
    w, h = b.size
    lobe = [(x, y) for y in range(0, h, 2) for x in range(0, w, 2)
            if px[x, y][1] > 150 and px[x, y][0] < 200 and px[x, y][2] < 200]
    if len(lobe) < 500:
        p.append("no lobe in the 2x image (%d green-ish pixels)" % len(lobe))
        return p
    # Pure lobe colour is (0,255,0); blended with white it is lighter.
    pure = sum(1 for x, y in lobe if px[x, y][0] < 30 and px[x, y][2] < 30)
    if pure > len(lobe) // 2:
        p.append("lobe is not blended (%d of %d pixels pure green)" % (pure, len(lobe)))
    # Screen door: a lobe pixel whose 4 neighbours are all background.
    isolated = 0
    for x, y in lobe[:4000]:
        if x < 2 or y < 2 or x >= w - 2 or y >= h - 2:
            continue
        around = [px[x + dx, y + dy] for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))]
        if all(c == (255, 255, 255) for c in around):
            isolated += 1
    if isolated > 20:
        p.append("%d isolated lobe pixels: a screen-door pattern" % isolated)
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "saveimage-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE.format(out=out))
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_OPEN_PANEL": "MOs",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out,
                                   "ECCE_VIEWER_SCENE_HOLD": "5"})
            log = result.log or ""
            keep = os.environ.get("SAVEIMAGE_KEEP")
            if keep:
                os.makedirs(keep, exist_ok=True)
                for f in os.listdir(out):
                    if f.endswith(".png"):
                        shutil.copy(os.path.join(out, f), keep)
            problems = check(out, log)
            print("%s  saveimage%s" % ("ok  " if not problems else "FAIL",
                                       "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in log.splitlines() if "SCENE" in l or "FAIL" in l)[-2000:])
                results.fail("saveimage", "; ".join(problems))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
