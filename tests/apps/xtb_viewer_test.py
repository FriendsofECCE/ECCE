#!/usr/bin/env python3
"""xtb results in the Builder, headless (#362).

Two real xtb calculations are made through Launch (tests/launch/xtb_test.py's
helpers), copied into a local data folder and opened with Builder scene
commands, no input events:

  * water GFN2-xTB energy (code and structure stored as the Calculation
    Editor stores them): the MOs panel's Compute draws an orbital (a grid
    with a non-zero range), the MO Diagram opens, is not refused as an
    orthonormal basis and a click on a level draws that MO, and the Density
    Map follows the click;
  * water GFN2-xTB --ohess: the Vibrational Frequencies panel lists the
    3 vibrations (no rigid-body modes), a mode shows its arrows in the viewer,
    and the Mulliken Charges panel opens.

    tests/apps/xtb_viewer_test.py [--build build-cmake]

Exit status 77 (skip) without xtb, Xvfb or the built tree.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "launch"))

SKIP = 77
failures = []


def check(ok, what):
    print("%s %s" % ("ok   " if ok else "FAIL ", what), flush=True)
    if not ok:
        failures.append(what)
    return ok


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(ROOT, "build-cmake"))
    args = ap.parse_args()
    if not shutil.which("xtb"):
        print("SKIP: xtb is not installed")
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    import harness
    import xtb_test as xt
    harness.prerequisites(os.path.abspath(args.build), ())

    #  Launch's Session points ECCE_TEST_HOME at a tree of links that has
    #  no Builder; the installed tree under test is put back afterwards.
    installed = os.environ.get("ECCE_TEST_HOME", "/opt/ecce")
    wrappers = os.environ.get("ECCE_TEST_WRAPPERS", "/usr/bin")
    if not os.path.exists(os.path.join(wrappers, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % wrappers)
        return SKIP

    #  Step 1: the calculations, through the real Launch.
    s = harness.Session(os.path.abspath(args.build), "xtbview",
                        {"xTB": shutil.which("xtb")}, local=True)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    calcs = {}
    try:
        if not s.services(True):
            print("FAIL services came up")
            return 1
        for key, runtype, level in (("sp", "Energy", "normal"),
                                    ("vib", "GeoVib", "normal")):
            text_, rc = xt.deck(xt.WATER, runtype, level)
            calcs[key] = xt.runJob(s, "%s-%d" % (key, int(time.time())),
                                   text_, work, rundir, xt.WATER)
    finally:
        s.stop()
    if not check(all(calcs.values()), "both xtb calculations completed"):
        return 1

    import apps
    apps.INSTALL = installed
    os.environ["ECCE_TEST_HOME"] = installed
    import isolate
    import orca_mo_test as base
    import xdisplay

    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    tmp = tempfile.mkdtemp(prefix="xtbview-",
                           dir=os.environ.get("XTBVIEW_KEEP") or
                           os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(tmp, "data")
    project = os.path.join(data, "users", "local", "xtb")
    os.makedirs(project)
    shutil.copy(base.PROJECT_META, os.path.join(project, ".ecce-meta"))
    os.environ["ECCE_LOCAL_DATA"] = data
    urls = {}
    for key, calc in calcs.items():
        dest = os.path.join(project, key)
        shutil.copytree(calc, dest, ignore=shutil.ignore_patterns(".DAV"))
        urls[key] = "file://" + dest + "/"
    xdisplay.SCREEN = "1700x1000x24"

    def run(display, key, name, scene, panel, env=None):
        out = os.path.join(tmp, name)
        res = base.runBuilder(display, urls[key], out, scene, panel=panel,
                              extraEnv=env)
        check(not res.crashed, "%s: the Builder did not crash" % name)
        failed = os.path.join(out, "FAILED")
        check(not os.path.exists(failed), "%s: the scene ran to the end %s" % (
            name, open(failed).read() if os.path.exists(failed) else ""))
        if os.environ.get("XTBVIEW_LOG"):
            print("\n".join("      | " + l for l in
                            (res.log or "").splitlines()[-40:]))
        return out, res.log or ""

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            out, log = run(display, "sp", "mo",
                           "hold 2\nmopanel compute\nmogrid grid\n", "MOs")
            rng = base.gridRange(out, "grid")
            check(rng is not None and rng[1] > rng[0],
                  "MOs: Compute draws an orbital, grid %s" % (rng,))
            check("ECCE_TEST_DIALOG" not in log, "MOs: no dialog was shown")

            scene = ("hold 3\nmodiagram click click 4\nmogrid grid\n"
                     "list Density Map\nhold 2\n"
                     "modiagram c1 click 3\ndmstate\n")
            out, log = run(display, "sp", "diagram", scene, "MO Diagram")
            check("grid yes" in text(out, "click"),
                  "MO Diagram: a click on a level draws that MO")
            check("orthonormalised" not in log and
                  "ECCE_TEST_DIALOG" not in log,
                  "MO Diagram: not refused as an orthonormal basis")
            check(re.search(r"DENSITYMAP: dmstate\n(.*\n)*?have 1", log)
                  is not None,
                  "Density Map: has a map for the xtb calculation")

            dump = os.path.join(tmp, "spectrum.dump")
            scene = ("hold 3\nvibfocus\nvibanim\nvibmode 1\nvibstep 3\n"
                     "vibvector\nvibstate\n")
            out, log = run(display, "vib", "vib", scene,
                           "Vibrational Frequencies",
                           {"ECCE_SPECTRUM_DUMP": dump})
            sticks = [l for l in open(dump)] if os.path.exists(dump) else []
            sticks = [l for l in sticks if l.startswith("stick ir")]
            check(len(sticks) == 3,
                  "Vibrations: 3 modes listed for water (%d)" % len(sticks))
            m = re.search(r"VIBSTATE: atoms=(\d+) arrows=(-?\d+)", log)
            check(m is not None and m.group(1) == "3" and int(m.group(2)) > 0,
                  "Vibrations: a mode shows its arrows on 3 atoms (%s)" %
                  (m.group(0) if m else "no VIBSTATE"))

            out, log = run(display, "vib", "charges", "hold 3\n",
                           "Mulliken Charges")
            check("no property panel named" not in log,
                  "Charges: the Mulliken Charges panel opens")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    if os.environ.get("XTBVIEW_KEEP"):
        print("kept " + tmp)
    else:
        shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
