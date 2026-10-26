#!/usr/bin/env python3
"""Relaxed Scan panel Show Graph/Table choice (#584), headless.

    tests/apps/scan_view_test.py [--build build-cmake] [--png DIR]

Opens the stored butane scan example.  Graph is the default (table hidden, plot
tall), Table shows the table, a table row click selects that point, and the
choice is remembered by the next panel.  Exit status 77 (skip) without Xvfb.
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

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
STORED = os.path.join(REPO, "data", "client", "examples", "organic",
                      "conformations", "ecceqm-butane-scan", "stored", "butane_scan")


def views(text):
    return re.findall(r"BUILDER: scanview table=(\d) plotShown=(\d) gridShown=(\d) "
                      r"plotH=(\d+) gridH=(\d+)", text)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb") or not shutil.which("import"):
        harness.skip("Xvfb/import missing")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "scanview", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
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
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "butane_scan")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(STORED, calc, symlinks=True)
        extra = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Relaxed Scan"}
        argv = ["-context", "file://" + calc + "/"]
        run1 = ["wait 6000", "scanview", "shot graph", "scanview table", "scanrow 3",
                "shot table", "scanstatus", "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, argv, run1, png,
                               "scanview1", extra)
        v = views(text)
        check(len(v) == 2, "scanview answered twice")
        if len(v) == 2:
            g, t = v
            check(g[:3] == ("0", "1", "0"), "default is Graph: plot shown, no table")
            check(t[:3] == ("1", "0", "1"), "Table shows the table, hides the plot")
            say("plot %s px in Graph, table %s px in Table" % (g[3], t[4]))
        check("scanrow 3 point=3" in text, "a table row click selects that point")
        # The choice made above must be what the next panel opens with.
        text, _ = bo.runScript(s, display, env, wrappers, argv,
                               ["wait 6000", "scanview", "shot again", "quit"], png,
                               "scanview2", extra)
        v = views(text)
        check(len(v) == 1 and v[0][:3] == ("1", "0", "1"),
              "the Table choice is remembered on reopening")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
