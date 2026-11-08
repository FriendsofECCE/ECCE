#!/usr/bin/env python3
"""The Clipping plane panel's plane cuts the structure open in the viewer (#879).

    tests/apps/clipplane_test.py              own Xvfb, services, local data
    tests/apps/clipplane_test.py --shots DIR  also keep the images there

Opens a Builder on the water fixture, a white background, and drives the
panel through its script hooks (clipplane, clipdir, clipoffset, clipflip):

  - plane normal to x through the middle: one half of the picture is bare
    background and the other still shows atoms; "Flip side" swaps them;
  - plane normal to z: the picture changes and still shows atoms;
  - off again: the picture is the one drawn before.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
GOOD = os.path.join(HERE, "fixtures", "calc-orca-water-ccpvdz")
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")

SCENE = """\
viewall
background 1 1 1
snap off0
clipplane on
clipdir x
clipoffset 0
clipinfo info_x
snap x_pos
clipflip
snap x_neg
clipflip
clipdir z
snap z_pos
clipplane off
snap off1
"""

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-12:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def ppm(path):
    with open(path, "rb") as handle:
        data = handle.read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    if not m:
        return 0, 0, b""
    return int(m.group(1)), int(m.group(2)), data[m.end():]


def inked(image, side):
    """Non-white pixels in the left or right part of the picture, leaving a
    margin about the middle where the plane is."""
    w, h, data = image
    margin = max(4, w // 25)
    lo, hi = (0, w // 2 - margin) if side == "left" else (w // 2 + margin, w)
    count = 0
    for y in range(h):
        row = data[(y * w + lo) * 3:(y * w + hi) * 3]
        for i in range(0, len(row), 3):
            if row[i] < 240 or row[i + 1] < 240 or row[i + 2] < 240:
                count += 1
    return count


def run(display, data, work, keep):
    project = os.path.join(data, "users", "local", "clip-test")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water")
    shutil.copytree(GOOD, calc)
    out = os.path.join(work, "out")
    os.makedirs(out)
    script = os.path.join(out, "scene")
    with open(script, "w") as handle:
        handle.write(SCENE)
    result = apps.run(display, "builder", args=("-context", "file://%s/" % calc),
                      windowTimeout=120, settle=90,
                      env={"ECCE_VIEWER_SCENE": script,
                           "ECCE_VIEWER_SCENE_OUT": out,
                           "ECCE_TRANSPARENCY_FALLBACK_MS": "0"})
    log = result.log or ""
    what = "Clipping plane"
    if result.crashed:
        return fail(what, "builder CRASHED (%s)" % result.signalName, log)
    failed = os.path.join(out, "FAILED")
    if os.path.exists(failed):
        return fail(what, "the script stopped: " + open(failed).read(), log)
    names = ("off0", "x_pos", "x_neg", "z_pos", "off1")
    images = {}
    for n in names:
        path = os.path.join(out, n + ".ppm")
        if not os.path.exists(path):
            return fail(what, "no picture " + n, log)
        images[n] = ppm(path)
    problems = []
    seen = {n: (inked(images[n], "left"), inked(images[n], "right"))
            for n in names}
    print("      inked pixels (left, right): %s" % seen)
    if min(seen["off0"]) == 0:
        problems.append("the plain picture has no atoms on one half: %s"
                        % (seen["off0"],))
    for n, empty in (("x_pos", 0), ("x_neg", 1)):
        # one half empty (which one depends on the side kept), other drawn
        if sorted(seen[n])[0] != 0 or sorted(seen[n])[1] == 0:
            problems.append("%s: one half should be bare background and the "
                            "other drawn: %s" % (n, seen[n]))
    if seen["x_pos"][0] == seen["x_neg"][0] and seen["x_pos"][0] == 0:
        problems.append("flipping the side left the same half bare")
    if images["z_pos"][2] == images["off0"][2] or not any(seen["z_pos"]):
        problems.append("plane normal to z: picture unchanged or empty")
    if images["off1"][2] != images["off0"][2]:
        problems.append("off again does not restore the picture")
    info = os.path.join(out, "info_x.txt")
    if not os.path.exists(info) or "on 1" not in open(info).read():
        problems.append("clipinfo does not report the plane on")
    if keep:
        os.makedirs(keep, exist_ok=True)
        for n in names:
            subprocess.run(["convert", os.path.join(out, n + ".ppm"),
                            os.path.join(keep, "clip-%s.png" % n)])
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: half the picture removed, side flips, off restores"
              % what)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="keep the pictures here")
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="clip-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, options.shots)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
