#!/usr/bin/env python3
"""Builder: a click on a measure's number opens its edit prompt (#716).

    tests/apps/builder_measure_label_test.py [--build DIR] [--keep]
Headless on benzene.car through ECCE_BUILDER_SCRIPT.  Measure distance,
angle and torsion are made as the Measure toolbar makes them; "labelclick"
sends a wx left click on the middle of the drawn number, and the script
hook logs the edit prompt instead of opening it.

Exit status 77 (skip) without Xvfb or the built Builder.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_edit_bugs_test as eb  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = eb.check


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "measurelabel", {}, local=True, keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        lines = ["wait 3000",
                 "measure dist 1 2", "wait 1500", "labelclick dist",
                 "measure angle 1 2 3", "wait 1500", "labelclick angle",
                 "measure torsion 1 2 3 4", "wait 1500", "labelclick torsion",
                 "quit"]
        text = eb.runBuilder(s, env, wrappers, "labels", lines)
        got = re.findall(r"BUILDER: measure popup (\w+) (\S+)", text)
        say("popups: %s" % (got,))
        if len(got) != 3:
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
        kinds = [k for k, _ in got]
        check(kinds == ["distance", "angle", "torsion"],
              "a click on the number opens the distance, angle and torsion "
              "prompts (%s)" % kinds)
        check("FAIL" not in text, "no script command failed")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
