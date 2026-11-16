#!/usr/bin/env python3
"""Diffraction property pane, automatic updates, breakout and pointer picking.

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
        path = lambda name: os.path.join(png, name)
        lines = ["wait 3000", "background 1 1 1", "xrd size 1366 768", "list Diffraction", "wait 1500", "panesize Diffraction",
                 "xrd wait", "xrd state", "xrd export %s %s" % (path("initial.png"), path("initial.csv")),
                 "xrd screen " + path("1013-pane-powder.png"),
                 "xrd set max 70 75 80",
                 "wait 1200", "xrd wait", "xrd state",
                 "xrd export %s %s" % (path("changed.png"), path("changed.csv")),
                 "xrd hover 31.7", "xrd click 31.7", "xrd selection", "xrd breakout 1", "wait 400", "xrd state",
                 "xrd export %s %s" % (path("breakout.png"), path("breakout.csv")),
                 "xrd breakout 0", "wait 300", "xrd state",
                 "xrd set view 2", "wait 1200", "xrd wait", "xrd hover 2 0 0",
                 "xrd set view 3", "wait 1200", "xrd wait", "xrd hover 2 0 -1",
                 "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines, png, "pane",
                               {"ECCE_PANEL_MODE": "detail"})
        open(path("pane.log"), "w").write(text)
        states = re.findall(r"BUILDER: xrd: view=(.*)", text)
        bo.check("PANESIZE Diffraction: shown" in text, "Properties entry opens the docked pane")
        bo.check(len(states) >= 4 and "docked=1" in states[0] and "dirty=0" in states[0],
                 "first pattern calculated on open")
        if len(states) >= 4:
            complete = lambda st: int(re.search(r"completed=(\d+)", st).group(1))
            bo.check(complete(states[1]) == complete(states[0]) + 1,
                     "rapid field edits debounce to one calculation without a button")
            bo.check("docked=0" in states[2] and "docked=1" in states[3] and
                     complete(states[2]) == complete(states[1]) == complete(states[3]),
                     "Pattern toggles out and back without recalculating")
        files = [path(n + ".csv") for n in ("initial", "changed", "breakout")]
        bo.check(all(os.path.isfile(f) for f in files) and
                 open(files[0]).read() != open(files[1]).read() and
                 open(files[1]).read() == open(files[2]).read(),
                 "changed range changes data; pane and breakout share the same result")
        bo.check("(2 0 0)" in text and "multiplicity = 6" in text,
                 "pointer-position hover/click reports the NaCl 200 reflection and multiplicity")
        bo.check("pinned=1 selected=2 0 0" in text, "click pins the tooltip and selects the reflection row")
        bo.check("|F| =" in text, "precession spot hover includes the structure factor")
        bo.check("refused:" not in text and "Unhandled" not in text, "pane hooks finish without exceptions")

        url = pc.importExample(s, env, wrappers, os.path.join(example, "xtb-b_dna", "bdna.out"))
        if not bo.check(bool(url), "DNA example imports"):
            return 1
        lines = ["wait 3000", "background 1 1 1", "xrd size 1366 768", "list Diffraction", "wait 3000", "xrd wait",
                 "xrd set repeat 42", "xrd set B 30", "xrd update", "xrd wait", "xrd state",
                 "xrd options", "wait 500", "xrd screen " + path("1013-options-fibre-screen.png"),
                 "xrd options-shot " + path("1013-options-fibre.png"), "xrd state",
                 "xrd export %s %s" % (path("fibre.png"), path("fibre.csv")),
                 "xrd set gamma 0.8", "xrd set scaling 1", "wait 400", "xrd state",
                 "xrd export %s %s" % (path("fibre-display.png"), path("fibre-display.csv")), "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", url], lines, png, "fibre",
                               {"ECCE_PANEL_MODE": "detail"})
        open(path("fibre.log"), "w").write(text)
        states = re.findall(r"BUILDER: xrd: view=(.*)", text)
        bo.check(len(states) == 3 and all("repeat=42" in st for st in states),
                 "typed pointer repeat survives calculation")
        bo.check(len(states) == 3 and len({re.search(r"generation=(\d+)", st).group(1) for st in states}) == 1,
                 "display gamma and scaling redraw without recalculation")
        bo.check(os.path.exists(path("fibre-display.csv")) and
                 open(path("fibre.csv")).read() == open(path("fibre-display.csv")).read() and
                 open(path("fibre.png"), "rb").read() != open(path("fibre-display.png"), "rb").read(),
                 "fibre display controls change the image but preserve raw data")

        # A delayed executable exercises the lifecycle without depending on
        # the speed of a particular structure or machine.
        tool = os.path.join(s.home, "bin", "ecce-xrd")
        real_tool = tool + "-real"
        shutil.move(tool, real_tool)
        with open(tool, "w") as f:
            f.write("#!" + sys.executable + "\nimport runpy, time\ntime.sleep(3)\n" +
                    "runpy.run_path(" + repr(real_tool) + ", run_name='__main__')\n")
        os.chmod(tool, 0o755)
        lines = ["wait 3000", "list Diffraction", "xrd wait", "xrd state",
                 "xrd set max 70", "wait 700", "xrd state", "xrd wait",
                 "xrd update", "xrd set max 80", "xrd state", "xrd update", "xrd wait", "xrd state",
                 "xrd export %s %s" % (path("slow.png"), path("slow.csv")), "quit"]
        powder_url = pc.importExample(s, env, wrappers, os.path.join(example, "qe-nacl_powder", "nacl_powder.pwout"))
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", powder_url], lines, png, "slow",
                               {"ECCE_PANEL_MODE": "detail"})
        open(path("slow.log"), "w").write(text)
        states = re.findall(r"BUILDER: xrd: view=(.*)", text)
        bo.check(len(states) == 4 and all("manual=1" in st for st in states) and
                 "dirty=1" in states[1] and "running=0" in states[1] and
                 "Update required" in text,
                 "a slow run switches to Update on request for this structure")
        bo.check(len(states) == 4 and "running=0" in states[2] and "dirty=1" in states[2] and
                 "completed=3" in states[3] and "dirty=0" in states[3],
                 "an edit cancels an in-flight run; only the latest Update completes")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    harness.say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
