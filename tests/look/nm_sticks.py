#!/usr/bin/env python3
"""Bond sticks are drawn thinner while normal-mode arrows are shown (#227).

    tests/look/nm_sticks.py BUILD/viewer-scenes

Scene script: radius before any arrows, with arrows (`nmtest`), after
`nmhide`, with arrows again, and after a style change while arrows are
shown.  The radius must be reduced exactly while the arrows are shown and
back to the original afterwards.  Exit 77 without the binary or Xvfb.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

SCENE = """style Ball And Stick
stickradius r0
nmtest
stickradius r1
nmhide
stickradius r2
nmtest
stickradius r3
style Stick
stickradius r4
nmhide
stickradius r5
style Ball And Stick
stickradius r6
"""


def main():
    if len(sys.argv) < 2 or not os.access(sys.argv[1], os.X_OK):
        print("SKIP: viewer-scenes not built (ninja viewer-scenes)")
        return 77
    import xdisplay
    try:
        xdisplay.findXvfb()
    except xdisplay.DisplayUnavailable as e:
        print("SKIP: %s" % e)
        return 77
    out = tempfile.mkdtemp(prefix="nmsticks")
    scene = os.path.join(out, "nm.scene")
    open(scene, "w").write(SCENE)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT)
    env.pop("FL_FONT_PATH", None)
    env["ECCE_REALUSERHOME"] = tempfile.mkdtemp(prefix="nmsticks-home")
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    with xdisplay.Display() as display:
        env["DISPLAY"] = ":%d" % display.number
        r = subprocess.run([os.path.abspath(sys.argv[1]), out, scene, "water"],
                           env=env, capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print("viewer-scenes failed:\n" + (r.stdout + r.stderr)[-1500:])
        return 1
    v = {k: float(open(os.path.join(out, k + ".txt")).read())
         for k in ("r0", "r1", "r2", "r3", "r4", "r5", "r6")}
    print(v)
    ok = (v["r0"] > 0 and v["r1"] < 0.5 * v["r0"] and abs(v["r2"] - v["r0"]) < 1e-6
          and abs(v["r3"] - v["r1"]) < 1e-6 and v["r4"] > 0
          and abs(v["r6"] - v["r0"]) < 1e-6)
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


sys.exit(main())
