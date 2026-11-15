#!/usr/bin/env python3
"""
Photo hooks of the Builder scene script (#860), checked on their state and
not on the picture: measure leaves the label drawn and no atom selected,
assigncharges stores the Mulliken charges on the atoms and labels them,
gtstep moves the Geometry Trace plot marker with the frame, panelfloat and
panelsize size the pane.  Water optimisation fixture, local data.
A second run without ECCE_VIEWER_SCENE_HOLD computes the HOMO and photographs
it with "shot": only the Builder may be mapped and its 3-D view must show
the surface's colour (#1056: a capture frame drew it over the Builder).

Same installed tree and isolation as atom_numbers_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without them.
"""

import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

CALCS = {"water": os.path.join(HERE, "fixtures", "calc-water-opt")}
SCENE = """windowsize 1500 1000
background 1 1 1
viewall
measure dist 0 1
measure angle 1 0 2
hold 1
assigncharges Mulliken Charges
gtstep 2
hold 1
gtframe 1
gtframe last
panelfloat MO Diagram 700 500
panelsize Geometry Trace 400 300
"""

MO_SCENE = """background 1 1 1
viewall
moselect 5
mopanel compute
shot mo
"""


def mappedWindows(env):
    """Viewable top-level windows of at least 100x100: [(x, y, w, h, title)]."""
    tree = subprocess.run(["xwininfo", "-root", "-children"], env=env, timeout=30,
                          capture_output=True, text=True).stdout
    found = []
    for wid, title in re.findall(r"^\s+(0x[0-9a-f]+) (\".*?\"|\(has no name\))", tree, re.M):
        info = subprocess.run(["xwininfo", "-id", wid], env=env, timeout=30,
                              capture_output=True, text=True).stdout
        if "IsViewable" not in info:
            continue
        g = [int(re.search(r"%s:\s+(-?\d+)" % k, info).group(1)) for k in
             ("Absolute upper-left X", "Absolute upper-left Y", "Width", "Height")]
        if g[2] >= 100 and g[3] >= 100:
            found.append(tuple(g) + (title,))
    return found


def photographMo(display, out, seen):
    """Answers the "shot mo" handshake: the mapped windows and the green
    (positive lobe) pixels inside the largest one, the Builder."""
    ready = os.path.join(out, "mo.ready")
    end = time.time() + 400
    while not os.path.exists(ready) and time.time() < end:
        time.sleep(0.3)
    if not os.path.exists(ready):
        return
    env = display.env()
    try:
        wins = mappedWindows(env)
        seen["windows"] = wins
        png = os.path.join(out, "mo.png")
        subprocess.run(["import", "-window", "root", png], env=env, timeout=60,
                       stderr=subprocess.DEVNULL)
        from PIL import Image
        x, y, w, h = max(wins, key=lambda b: b[2] * b[3])[:4] if wins else (0, 0, 0, 0)
        px = Image.open(png).convert("RGB").crop((x, y, x + w, y + h)).getdata()
        seen["green"] = sum(1 for r, g, b in px if g > 150 and r < 110 and b < 110)
    finally:
        open(os.path.join(out, "mo.go"), "w").close()


def checkMo(seen, log):
    p = []
    wins = seen.get("windows")
    if wins is None:
        return ["the MO shot never came"]
    if len(wins) != 1:
        p.append("windows mapped besides the Builder: %s" % sorted(wins, key=lambda b: -b[2] * b[3])[1:])
    if seen.get("green", 0) < 200:
        p.append("the Builder shows no MO surface (%s green pixels)" % seen.get("green"))
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def check(log):
    p = []
    got = re.findall(r"SHOTHOOK: measure (\w+) labels=(\d+) selected=(\d+)", log)
    if [g[0] for g in got] != ["dist", "angle"]:
        p.append("measure reports %s" % got)
    else:
        if int(got[1][1]) != 2:
            p.append("two measures should be drawn, %s are" % got[1][1])
        if any(int(g[2]) for g in got):
            p.append("atoms are still selected after measure")
    m = re.search(r"SHOTHOOK: assigncharges Mulliken Charges atoms=(\d+) first=(\S+) "
                  r"sum=(\S+) label1=\[([^\]]*)\]", log)
    if not m:
        p.append("no assigncharges line")
    else:
        if abs(float(m.group(2))) < 0.05:
            p.append("atom 1 got no charge (%s)" % m.group(2))
        if abs(float(m.group(3))) > 0.02:
            p.append("charges do not sum to zero (%s)" % m.group(3))
        if m.group(2)[:4].lstrip("-") not in m.group(4):
            p.append("label %s does not show charge %s" % (m.group(4), m.group(2)))
    steps = re.findall(r"GTSTRESS: step (\d+) -> playback (-?\d+), plot marker (-?\d+) of (\d+)", log)
    if [x[0] for x in steps] != ["2", "1", "4"]:
        p.append("gtstep/gtframe report %s" % steps)
    else:
        for x in steps:
            if x[1] != x[0] or x[2] != x[0]:
                p.append("frame %s: playback %s, plot marker %s" % x[:3])
    fl = re.search(r"SHOTHOOK: panelfloat MO Diagram floating=(\d) size=(\d+)x(\d+)", log)
    if not fl or fl.group(1) != "1" or abs(int(fl.group(2)) - 700) > 4:
        p.append("MO Diagram not floated at 700 wide: %s" % (fl.group(0) if fl else None))
    if not re.search(r"SHOTHOOK: panelsize Geometry Trace floating=\d size=(\d+)x", log):
        p.append("no panelsize line")
    if "FAIL" in log:
        p.append("a scene command failed")
    return p


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "shothooks-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        try:
            scene = os.path.join(folder, "scene")
            with open(scene, "w") as h:
                h.write(SCENE)
            out = os.path.join(folder, "shots")
            os.makedirs(out, exist_ok=True)
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_OPEN_PANEL": "Geometry Trace",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out,
                                   "ECCE_VIEWER_SCENE_HOLD": "5"})
            log = result.log or ""
            problems = check(log)
            if os.environ.get("SHOT_HOOKS_KEEP"):   # a place to look afterwards
                keep = os.environ["SHOT_HOOKS_KEEP"]
                os.makedirs(keep, exist_ok=True)
                with open(os.path.join(keep, "builder.log"), "w") as h:
                    h.write(log)
                for f in os.listdir(out):
                    shutil.copy(os.path.join(out, f), keep)
            print("%s  shot hooks%s" % ("ok  " if not problems else "FAIL",
                                        "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in log.splitlines()
                                if "SHOTHOOK" in l or "GTSTRESS" in l or "FAIL" in l or "SCENE" in l or "scene" in l)[-2500:])
                results.fail("shot_hooks", "; ".join(problems))

            #  The MO surface photographed without ECCE_VIEWER_SCENE_HOLD (#1056).
            with open(scene, "w") as h:
                h.write(MO_SCENE)
            seen = {}
            shooter = threading.Thread(target=photographMo, args=(display, out, seen))
            shooter.start()
            result = apps.run(display, "builder", args=("-context", urls["water"]),
                              windowTimeout=240, settle=300,
                              env={"ECCE_OPEN_PANEL": "MOs",
                                   "ECCE_VIEWER_SCENE": scene,
                                   "ECCE_VIEWER_SCENE_OUT": out})
            shooter.join(30)
            log = result.log or ""
            problems = checkMo(seen, log)
            print("%s  MO surface shot%s" % ("ok  " if not problems else "FAIL",
                                             "" if not problems else ": " + "; ".join(problems)))
            if problems:
                print("\n".join(l for l in log.splitlines() if "SCENE" in l or "FAIL" in l)[-2500:])
                results.fail("shot_hooks_mo", "; ".join(problems))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)

    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
