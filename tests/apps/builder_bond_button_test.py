#!/usr/bin/env python3
"""Builder: the Bond button of the mode toolbar (#462).

    tests/apps/builder_bond_button_test.py [--build DIR] [--keep] [--shots DIR]

Headless through ECCE_BUILDER_SCRIPT: two carbons, "toolbar bond" (a click on
the button) must leave Bond mode in effect with the button pressed, and a
click joins the carbons with a covalent bond; with the Build panel's kind set
to Metal-Pi and the button pressed again the bond made has order 0.5.
Exit status 77 (skip) without Xvfb or the built Builder.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import builder_edit_bugs_test as ebt  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = ebt.check

TWO_C = ["wait 3000", "select -1",
         "add C Tetrahedral 0 0 0", "select -1", "add C Tetrahedral 8 0 0"]
JOIN = ["select -1", "selectelem C", "addelem C", "click", "wait 500"]


def orders(text):
    return [float(v) for v in re.findall(r"BUILDER: bond \d+-\d+ C-C "
                                         r"order=(\S+)", text)]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(ebt.REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots")
    args = ap.parse_args()
    build_dir = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build_dir, ())
    if not os.access(os.path.join(build_dir, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build_dir)
    import xdisplay

    s = harness.Session(build_dir, "bondbutton", {}, local=True,
                        keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build_dir, exe),
                     os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build_dir, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    extra = {"ECCE_TEST_DIALOG_CLOSE": "1", "ECCE_TEST_DIALOG_ANSWER": "yes"}
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        shots = os.path.abspath(args.shots) if args.shots else None
        if shots:
            os.makedirs(shots, exist_ok=True)
        lines = (TWO_C + ["bondkind covalent", "mode select", "info",
                          "toolbar bond", "info"] + JOIN + ["bondorders"])
        if shots:
            lines.append("shot bondbutton")
        text = ebt.runBuilder(s, env, wrappers, "single", lines + ["quit"],
                              shots, extra, structure=None)
        modes = re.findall(r"BUILDER: mode=(\S+) toolbar=(\S+) checked=(\d)",
                           text)
        check(modes[:2] == [("select", "select", "1"), ("bond", "bond", "1")],
              "pressing Bond puts the mode and the toolbar in Bond (%s)"
              % modes)
        check(orders(text) == [1.0],
              "covalent kind: one single C-C bond (%s)" % orders(text))
        text = ebt.runBuilder(s, env, wrappers, "metal", TWO_C + [
            "bondkind metal", "mode select", "toolbar bond"] + JOIN + [
            "bondorders", "quit"], None, extra, structure=None)
        check(orders(text) == [0.5],
              "Metal-Pi kind: the button makes a 0.5 bond (%s)"
              % orders(text))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(ebt.failures) if ebt.failures else "PASSED")
    return 1 if ebt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
