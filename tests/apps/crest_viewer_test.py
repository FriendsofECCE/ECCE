#!/usr/bin/env python3
"""A CREST ensemble in the Builder, headless (#682).

n-butane is searched with GFN-FF through the real Launch (crest_test.py's
helpers), the stored calculation is opened in the Builder's Geometry Trace
panel, and the window is photographed.  The panel must open (no "no property
panel named" in the log) and the Builder must not crash.

    tests/apps/crest_viewer_test.py [--build build-cmake] [--png DIR] [--keep]

Exit status 77 (skip) without crest, xtb, Xvfb, import or the built tree.
"""
import argparse
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import crest_test as ct  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("crest", "xtb", "Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "crestview",
                        {"xTB": shutil.which("xtb"),
                         "CREST": shutil.which("crest")},
                        local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)

    display = None
    try:
        if not s.services(True):
            ct.check(False, "services came up")
            return 1
        text, _ = ct.deck(ct.BUTANE)
        calc = ct.runJob(s, "butane-%d" % int(time.time()), text, work,
                         rundir, ct.BUTANE)
        if not calc:
            return 1
        project = os.path.join(s.localData(), "users", "local", "crest")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        dest = os.path.join(project, "butane")
        shutil.copytree(calc, dest, ignore=shutil.ignore_patterns(".DAV"))
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        out, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + dest + "/"],
            ["wait 4000", "shot ensemble", "quit"], png, "ensemble",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Conformers"})
        ct.check("ensemble" in shots, "the window was photographed")
        ct.check("no property panel named" not in out,
                 "the Builder offers a Conformers panel")
        ct.check("segmentation" not in out.lower() and
                 "crash" not in out.lower(), "no crash in the Builder log")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("screenshots: %s" % png)
    say("FAILED: " + "; ".join(ct.failures) if ct.failures else "PASSED")
    return 1 if ct.failures else 0


if __name__ == "__main__":
    sys.exit(main())
