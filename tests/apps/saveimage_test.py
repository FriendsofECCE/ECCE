#!/usr/bin/env python3
"""
Print-quality image export (#1059): the water HOMO saved with the scene hook
"saveimage <png> <factor> white" in the Builder.  The 2x image must be twice
the view's size, show the lobe in a colour blended with the white background
(not the pure lobe colour), and have no screen-door grid inside the lobe.
A "rotatex"/"rotate" turn followed at once by saveimage must keep the atoms
as bright as before the turn: the headlight follows the camera (#1075).
The Depth cueing and Perspective toggles (#1214): on, off and on again
through their handler; off gives back the first picture.

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
LABEL_SCENE = """windowsize 900 700
background 1 1 1
viewall
hold 2
saveimage {out}/n1.png 1
saveimage {out}/n2.png 2
labels element
hold 1
saveimage {out}/l1.png 1
saveimage {out}/l2.png 2
"""
MENU_SCRIPT = """saveimagemenu 2 white {out}/m2.png
saveimage {out}/b1.png 1 white
quit
"""
SCENE = """windowsize 900 700
background 1 1 1
viewall
moselect 5
mopanel compute
hold 2
surfmesh m30a
saveimage {out}/x1.png 1 white
saveimage {out}/x2.png 2 white
surfmesh m30b
"""

# #1075: a hook rotation followed at once by saveimage (no paint between)
# must be lit by the headlight at the new camera, as a mouse turn is.
LIGHT_SCENE = """windowsize 900 700
background 0 0 0
viewall
hold 2
saveimage {out}/r0.png 1
rotatex 90
saveimage {out}/r90.png 1
rotate 90
saveimage {out}/ry.png 1
"""
def litMean(path):
    """Mean luminance of the drawn (non-background) pixels on black."""
    from PIL import Image
    px = [v for v in Image.open(path).convert("L").getdata() if v > 8]
    return (sum(px) / len(px) if px else 0.0), len(px)


# #1090: without ECCE_VIEWER_SCENE_HOLD the hooks turn a capture viewer; the
# image must come from that viewer, so a turn must change the picture.
CAPTURE_SCENE = """viewall
hold 2
saveimage {out}/c0.png 1 white
rotatex 90
saveimage {out}/c90.png 1 white
zoom 2
saveimage {out}/cz.png 1 white
"""
ORTHO_SCENE = """viewall
hold 2
ortho on
viewall
camera ortho
saveimage {out}/o1.png 1 white
zoom 2
camera orthozoom
ortho off
camera persp
"""


# #1214: on/off/on of both toggles through the toolbar's handler.
TOGGLE_SCENE = """windowsize 900 700
background 1 1 1
viewall
hold 2
saveimage {out}/t0.png 1
depthcue on
hold 1
saveimage {out}/t1.png 1
depthcue off
hold 1
saveimage {out}/t2.png 1
depthcue on
hold 1
saveimage {out}/t3.png 1
depthcue off
ortho on
hold 1
camera tortho
saveimage {out}/t4.png 1
ortho off
hold 1
camera tpersp
saveimage {out}/t5.png 1
"""


def diffMean(a, b):
    """Mean absolute pixel difference (0..255) of two images."""
    from PIL import Image, ImageChops, ImageStat
    x = Image.open(a).convert("L")
    y = Image.open(b).convert("L")
    if x.size != y.size:
        return 255.0
    return ImageStat.Stat(ImageChops.difference(x, y)).mean[0]


def drawnMean(path):
    """Mean luminance of the pixels that are not the white background."""
    from PIL import Image
    px = [v for v in Image.open(path).convert("L").getdata() if v < 247]
    return (sum(px) / len(px) if px else 255.0), len(px)


def checkToggles(out, log):
    import re
    p = []
    f = lambda n: os.path.join(out, n + ".png")
    try:
        same, on, again = diffMean(f("t0"), f("t2")), diffMean(f("t0"), f("t1")), \
            diffMean(f("t1"), f("t3"))
        d0, n0 = drawnMean(f("t0"))
        d1, n1 = drawnMean(f("t1"))
        persp, ortho = diffMean(f("t0"), f("t5")), diffMean(f("t0"), f("t4"))
    except OSError as e:
        return ["missing image: %s" % e]
    print("      depthcue: on vs off %.3f, off again vs first %.3f, on again vs on %.3f;"
          " drawn mean %.1f -> %.1f" % (on, same, again, d0, d1))
    print("      ortho: on vs first %.3f, off again vs first %.3f" % (ortho, persp))
    if on < 0.05:
        p.append("depth cueing on did not change the picture")
    if same > 0.02:
        p.append("depth cueing off did not restore the picture (%.3f)" % same)
    if again > 0.02:
        p.append("depth cueing on again differs from the first on (%.3f)" % again)
    if d1 < d0 - 1.0:
        p.append("depth cueing darkened the molecule on white (%.1f -> %.1f)" % (d0, d1))
    if ortho < 0.05:
        p.append("orthographic on did not change the picture")
    if persp > 0.02:
        p.append("perspective back did not restore the picture (%.3f)" % persp)
    cams = dict(re.findall(r"SCENE camera (\S+):.* type (\w+)", log))
    if cams.get("tortho") != "orthographic" or cams.get("tpersp") != "perspective":
        p.append("camera types %s" % cams)
    if "no handler" in log or log.count("via the viewer's handler") < 6:
        p.append("a toggle did not reach the viewer's handler")
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def checkOrtho(out, log):
    import re
    p = []
    cams = dict(re.findall(r"SCENE camera (\S+):.* type (\w+)", log))
    sizes = dict(re.findall(r"SCENE camera (\S+):.* size ([0-9.e+-]+)", log))
    if cams.get("ortho") != "orthographic":
        p.append("ortho on: camera type %s" % cams.get("ortho"))
    if cams.get("persp") != "perspective":
        p.append("ortho off: camera type %s" % cams.get("persp"))
    try:
        if not float(sizes["orthozoom"]) < float(sizes["ortho"]) * 0.75:
            p.append("zoom 2 did not shrink the orthographic height")
    except (KeyError, ValueError):
        p.append("no camera lines")
    if not os.path.exists(os.path.join(out, "o1.png")):
        p.append("no orthographic image")
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def checkCapture(out, log):
    from PIL import Image, ImageChops
    p = []
    try:
        a, b, c = (Image.open(os.path.join(out, n + ".png")).convert("RGB")
                   for n in ("c0", "c90", "cz"))
        if not ImageChops.difference(a, b).getbbox():
            p.append("rotatex did not change the saved image")
        if not ImageChops.difference(b, c).getbbox():
            p.append("zoom did not change the saved image")
    except OSError as e:
        p.append("missing image: %s" % e)
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def checkLight(out, log):
    p = []
    try:
        ref, n0 = litMean(os.path.join(out, "r0.png"))
        for n in ("r90", "ry"):
            m, k = litMean(os.path.join(out, n + ".png"))
            print("      %s: mean %.1f over %d px (before the turn %.1f over %d)"
                  % (n, m, k, ref, n0))
            if k < 200 or abs(m - ref) > 0.15 * ref:
                p.append("%s mean brightness %.1f, %.1f before the turn" % (n, m, ref))
    except OSError as e:
        p.append("missing image: %s" % e)
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


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
    p += checkFine(out, log)
    return p


def checkFine(out, log):
    """#1075: 2x recomputes the surface on a 60 grid for the render only."""
    import re
    p = []
    if not re.search(r"MOPANEL: fine grid 60 ", log):
        p.append("2x did not compute a 60-point grid")
    n = {int(k): int(v) for k, v in re.findall(
        r"saveimage x(\d+) surface indices (\d+)", log)}
    if 2 not in n or n[2] <= 0:
        p.append("no surface indices reported for 2x")
    try:
        a = open(os.path.join(out, "m30a.txt")).read()
        b = open(os.path.join(out, "m30b.txt")).read()
        print("      surface at 2x: %s indices; on screen before: %s"
              % (n.get(2), a.splitlines()[1] if len(a.splitlines()) > 1 else a))
        if a != b:
            p.append("the on-screen surface changed over the save: %r -> %r"
                     % (a.splitlines()[1:2], b.splitlines()[1:2]))
    except OSError as e:
        p.append("missing surfmesh: %s" % e)
    return p


def nonWhite(path, box=None):
    from PIL import Image, ImageChops
    im = Image.open(path).convert("RGB")
    if box:
        im = im.crop(box)
    diff = ImageChops.difference(im, Image.new("RGB", im.size, (255, 255, 255)))
    return sum(1 for v in diff.convert("L").getdata() if v > 40)


def checkText(out, log):
    p = []
    from PIL import Image
    try:
        n1, n2, l1, l2 = (os.path.join(out, n + ".png") for n in ("n1", "n2", "l1", "l2"))
        corner = nonWhite(n1, (0, 0, 200, 40))
        if corner:
            p.append("corner text is in the 1x image (%d pixels)" % corner)
        label1 = nonWhite(l1) - nonWhite(n1)
        label2 = nonWhite(l2) - nonWhite(n2)
        if label1 < 20:
            p.append("no atom labels in the 1x image (%d pixels)" % label1)
        elif label2 < 2.5 * label1:
            p.append("labels do not scale: %d pixels at 1x, %d at 2x" % (label1, label2))
        print("      label pixels: 1x %d, 2x %d" % (label1, label2))
    except (OSError, FileNotFoundError) as e:
        p.append("missing image: %s" % e)
    return p


def checkMenu(out, log):
    from PIL import Image
    p = []
    try:
        m = Image.open(os.path.join(out, "m2.png")).size
        b = Image.open(os.path.join(out, "b1.png")).size
    except OSError as e:
        return ["missing image: %s" % e]
    if m != (2 * b[0], 2 * b[1]):
        p.append("menu 2x image is %s, builder-script 1x is %s" % (m, b))
    if "FAIL" in log:
        p.append("a script command failed")
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

            for tag, text, checker, extra in (
                    ("saveimage_light", LIGHT_SCENE, checkLight, {}),
                    ("saveimage_capture", CAPTURE_SCENE, checkCapture,
                     {"ECCE_VIEWER_SCENE_HOLD": None}),
                    ("saveimage_ortho", ORTHO_SCENE, checkOrtho, {}),
                    ("saveimage_toggles", TOGGLE_SCENE, checkToggles, {}),):
                with open(scene, "w") as h:
                    h.write(text.format(out=out))
                env = {"ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": out,
                       "ECCE_VIEWER_SCENE_HOLD": "5"}
                env.update(extra)
                env = {k: v for k, v in env.items() if v is not None}
                result = apps.run(display, "builder", args=("-context", urls["water"]),
                                  windowTimeout=240, settle=300, env=env)
                problems = checker(out, result.log or "")
                print("%s  %s%s" % ("ok  " if not problems else "FAIL", tag,
                                    "" if not problems else ": " + "; ".join(problems)))
                if problems:
                    print("\n".join(l for l in (result.log or "").splitlines()
                                    if "SCENE" in l or "FAIL" in l)[-1500:])
                    results.fail(tag, "; ".join(problems))

            with open(scene, "w") as h:
                h.write(LABEL_SCENE.format(out=out))
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out,
                                   "ECCE_VIEWER_SCENE_HOLD": "5"})
            problems = checkText(out, result.log or "")
            print("%s  saveimage text%s" % ("ok  " if not problems else "FAIL",
                                            "" if not problems else ": " + "; ".join(problems)))
            if problems:
                results.fail("saveimage_text", "; ".join(problems))

            script = os.path.join(folder, "bscript")
            with open(script, "w") as h:
                h.write(MENU_SCRIPT.format(out=out))
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_BUILDER_SCRIPT": script})
            problems = checkMenu(out, result.log or "")
            print("%s  saveimage menu and builder script%s" % ("ok  " if not problems else "FAIL",
                                "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in (result.log or "").splitlines() if "SCRIPT" in l)[-1500:])
                results.fail("saveimage_menu", "; ".join(problems))
            if keep:
                for f in os.listdir(out):
                    if f.endswith(".png"):
                        shutil.copy(os.path.join(out, f), keep)
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
