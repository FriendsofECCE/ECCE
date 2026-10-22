#!/usr/bin/env python3
"""Relaxed Scan panel on a running job (#460), headless.

    tests/apps/scan_panel_live_test.py [--build build-cmake] [--keep]

A live job delivers SCANCOORD and SCANENERGY first and SCANGEOM (parsed from
a separate file) at the end.  The Builder opens the calculation with only the
energies, so the panel is built without structures; then SCANGEOM is written
and the script command "propchange SCANGEOM" does what the job's property
message does.  Playback over every point must then put each point's stored
structure in the viewer.  Exit status 77 (skip) without Xvfb or binaries.
"""

import argparse
import os
import re
import shutil
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import scan_panel_test as sp  # noqa: E402
from harness import say  # noqa: E402

check = bo.check


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "scanlive", {}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    coord, energy = sp.golden("SCANCOORD"), sp.golden("SCANENERGY")
    frames = sp.allxyzFrames()
    n = len(frames)
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
        calc = os.path.join(project, "calc-h2o2-live")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(sp.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(sp.H2O2)
        with open(os.path.join(props, "SCANCOORD"), "w") as f:
            f.write(sp.vector("SCANCOORD", coord, "Degree"))
        with open(os.path.join(props, "SCANENERGY"), "w") as f:
            f.write(sp.vector("SCANENERGY", energy, "Hartree"))

        # The structures arrive while the Builder waits at "shot arrive".
        work = os.path.join(s.state, "scanlive")
        ready = os.path.join(work, "arrive.ready")

        def arrive():
            end = time.time() + 120
            while time.time() < end and not os.path.exists(ready):
                time.sleep(0.1)
            with open(os.path.join(props, "SCANGEOM"), "w") as f:
                f.write(sp.tsvector(frames))
        writer = threading.Thread(target=arrive, daemon=True)
        writer.start()

        lines = ["wait 6000", "scanstep first", "shot arrive",
                 "propchange SCANCOORD", "propchange SCANENERGY",
                 "propchange SCANGEOM", "scanstep first"]
        lines += ["scanstep next"] * (n - 1)
        lines += ["quit"]
        text, _ = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc],
            lines, png, "scanlive",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Relaxed Scan"})
        writer.join(1)
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        check("BUILDER: propchange SCANGEOM" in text, "SCANGEOM arrived")
        seq = [(m[0], int(m[1]), m[2:]) for m in re.findall(
            r"BUILDER: scanstep (\S+) point=(-?\d+) row=-?\d+ "
            r"marker=-?\d+ atoms: (\S+) (\S+) (\S+) (\S+) (\S+) (\S+)", text)]
        check(len(seq) == n + 1, "%d scanstep commands answered (%d)"
              % (n + 1, len(seq)))
        before, after = seq[0], seq[1:]
        start = [float(v) for v in before[2]]
        want0 = frames[0][0] + frames[0][1]
        check(not all(abs(a - b) < 2e-5 for a, b in zip(start, want0)),
              "before the structures arrive the viewer keeps the input geometry")
        for i, (arg, point, atoms) in enumerate(after):
            check(point == i, "step %d selects point %d (got %d)" % (i, i, point))
            xyz = frames[i][0] + frames[i][1]
            check(all(abs(float(a) - b) < 2e-5 for a, b in zip(atoms, xyz)),
                  "point %d: the viewer holds that point's stored structure" % i)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
