#!/usr/bin/env python3
"""Diffraction pane is not computed until selected; one run after rapid edits; Cancel (#1245, #1246).

Uses the installed test tree, isolated services and Xvfb. --png keeps screen
captures of the docked pane and fibre Options on a 1366x768 display.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "launch"))
import builder_overlay_test as bo
import harness
import packing_cells_test as pc


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--build", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--png")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())
    import xdisplay
    s = harness.Session(build, "diffraction-panel", {}, local=True)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in pc.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    shutil.copy(os.path.join(ROOT, "scripts", "ecce_xrd.py"), os.path.join(s.home, "bin", "ecce-xrd"))
    os.chmod(os.path.join(s.home, "bin", "ecce-xrd"), 0o755)
    wrappers = os.path.join(s.state, "wrappers")
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            bo.check(False, "isolated services start")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        example = os.path.join(ROOT, "data", "client", "examples", "genchem", "diffraction")
        url = pc.importExample(s, env, wrappers, os.path.join(example, "qe-nacl_powder", "nacl_powder.pwout"))
        if not bo.check(bool(url), "NaCl example imports"):
            return 1
        lines = ["wait 3000", "xrd size 1366 768", "panesize Diffraction",
                 "list Diffraction", "wait 1500", "panesize Diffraction",
                 "xrd wait", "xrd state",
                 "xrd set max 70 75 80", "wait 1200", "xrd wait", "xrd state",
                 "xrd update", "xrd cancel", "xrd progress", "xrd state",
                 "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines, png, "nodefault",
                               {"ECCE_PANEL_MODE": "detail"})
        sizes = re.findall(r"PANESIZE Diffraction: (\w+)", text)
        states = re.findall(r"BUILDER: xrd: view=(.*)", text)
        bo.check(len(sizes) >= 2 and sizes[0] == "hidden",
                 "Diffraction is not the default pane (nothing computes before it is selected)")
        bo.check(len(sizes) >= 2 and sizes[1] == "shown", "selecting it opens the pane")
        if len(states) >= 3:
            complete = lambda st: int(re.search(r"completed=(\d+)", st).group(1))
            bo.check(complete(states[1]) == complete(states[0]) + 1,
                     "rapid edits give one new run, not a queue")
            bo.check("running=0" in states[-1] and "dirty=1" in states[-1],
                     "Cancel kills the run and leaves nothing running")
        else:
            bo.check(False, "xrd states reported")
        bo.check("progress=0 running=0" in text, "progress row hidden after Cancel")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    harness.say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
