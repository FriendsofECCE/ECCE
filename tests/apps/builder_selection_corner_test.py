#!/usr/bin/env python3
"""Builder: after a click on an atom, and a Shift-click on a second one, the
bottom-left corner of the 3-D view shows the selection's details as pixels,
on a black and on a white background (#1236).

    tests/apps/builder_selection_corner_test.py [--build build-cmake] [--png DIR]

The clicks are wx mouse events on the render area (script "atomclick"), the
path a real click takes; the corner is photographed and its pixels compared
with the corner before the click.  "corner" prints the strings the viewer
holds, so a failure says whether the text was missing or not drawn.

Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or the built
binaries.
"""

import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)),
                                "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

STRIP = 30   # bottom rows of the view that hold the lower-left text


def strip(path, box):
    from PIL import Image
    import numpy
    img = Image.open(path).convert("RGB")
    x0, y0, x1, y1 = box
    return numpy.asarray(img.crop((x0, y1 - STRIP, x0 + (x1 - x0) // 2,
                                   y1))).astype(int)


def ink(px, background):
    """Pixels clearly unlike the background colour (0 black, 255 white)."""
    import numpy
    return int((numpy.abs(px - background).max(axis=2) > 80).sum())


def changed(a, b):
    import numpy
    return int((numpy.abs(a - b).max(axis=2) > 80).sum())


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(bo.REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    problems = []
    s = harness.Session(build, "selcorner", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    harness.link(os.path.join(build, "builder"), os.path.join(s.home, "bin", "builder"))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binary: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    text, shots = "", {}
    try:
        if not s.services(True):
            print("FAILED: services did not come up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "selcorner")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-opt")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(bo.FIXTURE, calc, symlinks=True)
        lines = ["wait 4000", "background 0 0 0", "mode select", "wait 1000",
                 "corner", "shot plain",
                 "atomclick none 1", "wait 1500", "corner", "shot one",
                 "atomclick shift 2", "wait 1500", "corner", "shot two",
                 "background 1 1 1", "wait 1500", "corner", "shot white",
                 "quit"]
        text, shots = bo.runScript(s, display, env, wrappers,
                                   ["-context", "file://" + calc + "/"],
                                   lines, png, "selcorner")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()

    corners = re.findall(r"^BUILDER: corner (.*)$", text, re.M)
    picks = re.findall(r"^BUILDER: selected after .*$", text, re.M)
    for line in picks + corners:
        say("  " + line)
    if len(corners) != 4:
        problems.append("expected 4 corner reports, got %d" % len(corners))
    else:
        if "Å" not in corners[1] or "Å" not in corners[2]:
            problems.append("the viewer holds no selection text: %s | %s"
                            % (corners[1], corners[2]))
    if set(shots) != {"plain", "one", "two", "white"}:
        problems.append("photos missing: have %s" % sorted(shots))
    else:
        from PIL import Image
        box = bo.viewerBox(Image.open(shots["plain"]))
        if box is None:
            problems.append("no black 3-D view in the first photo")
        else:
            say("  view box %s" % (box,))
            plain = strip(shots["plain"], box)
            counts = {"plain ink": ink(plain, 0)}
            for name in ("one", "two"):
                px = strip(shots[name], box)
                counts[name + " ink"] = ink(px, 0)
                counts[name + " changed"] = changed(plain, px)
            counts["white ink"] = ink(strip(shots["white"], box), 255)
            say("  pixels in the bottom-left strip: %s" % counts)
            for name in ("one", "two"):
                if counts[name + " ink"] < 40 or counts[name + " changed"] < 40:
                    problems.append("after the %s-atom click the bottom-left "
                                    "corner shows no new text (%s ink, %s changed)"
                                    % (name, counts[name + " ink"],
                                       counts[name + " changed"]))
            if counts["white ink"] < 40:
                problems.append("on white the bottom-left text is not drawn "
                                "(%d ink)" % counts["white ink"])
    say("screenshots: %s" % png)
    print("FAILED: " + "; ".join(problems) if problems else "PASSED")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
