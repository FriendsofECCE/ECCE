#!/usr/bin/env python3
"""ORCA orbitals in the Builder's MOs panel, headless (#239).

    tests/apps/orca_mo_test.py                  own Xvfb, services, local data
    tests/apps/orca_mo_test.py --shots DIR      also save the window as PNGs

Two calculations, both water with cc-pVDZ as ECCE holds it (24 functions):

  calc-orca-water-ccpvdz   ORCA 6.1.1 output, 24 functions; trace(P.S) = 10
                           checked with ecce-mocomp.  Compute must draw the
                           orbital (a grid with a non-zero range), with no
                           dialog and no width fallback in the log.
  ... with orca-mo-23/     the coefficients of an ORCA run whose input lost
                           a contraction of each generally contracted shell
                           (ECCE before 9.0.0-alpha.7): 23 functions.
                           Compute, twice, must give ONE dialog saying the
                           numbers of functions differ, never the "grid
                           value is 0" text.

And, in every panel layout, the MOs pane no narrower than its own rows.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

FIXTURES = os.path.join(HERE, "fixtures")
GOOD = os.path.join(FIXTURES, "calc-orca-water-ccpvdz")
BAD_PROPS = os.path.join(FIXTURES, "orca-mo-23")
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
MISMATCH = ("the output has 23 basis functions, but the basis set ECCE has "
            "for this calculation has 24")
ZERO_GRID = "The grid value for this MO is 0"
SKIP = 77

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-12:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def traceCheck():
    """trace(P.S) = sum over occupied MOs of occ * c.S.c, from ecce-mocomp."""
    mocomp = os.path.join(apps.WRAPPERS, "ecce-mocomp")
    total = 0.0
    for mo in range(1, 6):
        out = subprocess.run([mocomp, GOOD, "--mo", str(mo)],
                             capture_output=True, text=True).stdout
        occ = re.search(r"occ ([0-9.]+)", out)
        csc = re.search(r"c\.S\.c = ([0-9.]+)", out)
        if not occ or not csc:
            fail("trace(P.S)", "ecce-mocomp gave no c.S.c for MO %d" % mo, out)
            return
        total += float(occ.group(1)) * float(csc.group(1))
    if abs(total - 10.0) > 1e-3:
        fail("trace(P.S)", "%.4f, not the 10 electrons of water" % total)
    else:
        print("ok    trace(P.S) = %.4f for water's 10 electrons" % total)


def makeCalc(data, name, props=None):
    project = os.path.join(data, "users", "local", "orca-mo")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, name)
    shutil.copytree(GOOD, calc)
    if props:
        for f in os.listdir(props):
            shutil.copy(os.path.join(props, f), os.path.join(calc, "Props"))
    return "file://" + calc + "/"


def runBuilder(display, url, out, scene, shot=None, mode=None,
               dialogSeconds="1"):
    os.makedirs(out, exist_ok=True)
    script = os.path.join(out, "scene")
    with open(script, "w") as handle:
        handle.write(scene)

    def inspect(disp):
        if not shot:
            return
        #  The screen, cut to the largest window (the Builder's frame):
        #  importing a GL-backed frame by id fails under Xvfb.
        best = None
        for wid, _title in disp.windows():
            info = subprocess.run(["xwininfo", "-display", disp.name, "-id",
                                   wid], capture_output=True,
                                  text=True).stdout
            geo = dict(re.findall(r"(Absolute upper-left [XY]|Width|Height):"
                                  r"\s+(-?\d+)", info))
            if len(geo) == 4 and (best is None or
                                  int(geo["Width"]) * int(geo["Height"]) >
                                  int(best["Width"]) * int(best["Height"])):
                best = geo
        argv = ["import", "-display", disp.name, "-window", "root"]
        if best:
            argv += ["-crop", "%sx%s+%s+%s" % (
                best["Width"], best["Height"], best["Absolute upper-left X"],
                best["Absolute upper-left Y"]), "+repage"]
        subprocess.run(argv + [shot], check=False)

    env = {"ECCE_PANEL_MODE": mode} if mode else {}
    if shot:
        #  The scene in the Builder's own canvas, the window kept open.
        env["ECCE_VIEWER_SCENE_HOLD"] = "120"
    return apps.run(display, "builder", args=("-context", url),
                    windowTimeout=120, settle=90 if not shot else 40,
                    inspect=inspect if shot else None,
                    env=dict(env, **{"ECCE_OPEN_PANEL": "MOs",
                         "ECCE_VIEWER_SCENE": script,
                         "ECCE_VIEWER_SCENE_OUT": out,
                         "ECCE_TEST_DIALOG_CLOSE": dialogSeconds,
                         "ECCE_PANEL_METRICS": os.path.join(out,
                                                            "metrics.txt"),
                         "ECCE_TRANSPARENCY_FALLBACK_MS": "0"}))


def gridRange(out, name):
    path = os.path.join(out, name + ".txt")
    if not os.path.exists(path):
        return None
    text = open(path).read()
    m = re.search(r"fieldMin (\S+)\s+fieldMax (\S+)", text)
    return (float(m.group(1)), float(m.group(2))) if m else (0.0, 0.0)


def checkGood(display, data, work):
    url = makeCalc(data, "water-24")
    out = os.path.join(work, "good")
    result = runBuilder(display, url, out, "mopanel compute\nmogrid grid\n")
    log = result.log or ""
    rng = gridRange(out, "grid")
    what = "24-function ORCA calculation"
    if result.crashed:
        fail(what, "builder CRASHED (%s)" % result.signalName, log)
    elif os.path.exists(os.path.join(out, "FAILED")):
        fail(what, "scene stopped: " + open(os.path.join(out, "FAILED")).read(),
             log)
    elif rng is None:
        fail(what, "the scene never reported the grid", log)
    elif not rng[1] > rng[0]:
        fail(what, "the orbital's grid is empty (%g..%g)" % rng, log)
    elif "ECCE_TEST_DIALOG" in log:
        fail(what, "a dialog was shown", log)
    elif "coefficient width" in log:
        fail(what, "the width fallback fired: the basis is recorded with the "
             "wrong convention", log)
    else:
        print("ok    %s: orbital drawn, grid %.3g..%.3g" % (what, rng[0],
                                                           rng[1]))


def checkWidth(display, data, work, mode):
    """The MOs pane at least as wide as its rows.  This calculation has
    Mulliken charges, so the field-type choice carries its longest entry
    ("Density (ESP from atomic charges)") as a finished run's does."""
    url = makeCalc(data, "water-" + mode)
    out = os.path.join(work, "width-" + mode)
    result = runBuilder(display, url, out, "hold 20\n", mode=mode)
    what = "MOs pane width, %s layout" % mode
    path = os.path.join(out, "metrics.txt")
    m = None
    if os.path.exists(path):
        m = re.search(r'pane "MOs" .* need (\d+) have (\d+)', open(path).read())
    if not m:
        fail(what, "no metrics for the MOs pane", result.log)
    elif int(m.group(2)) < int(m.group(1)):
        fail(what, "%s px wide, its controls need %s: clipped at the right"
             % (m.group(2), m.group(1)))
    else:
        print("ok    %s: %s px, controls need %s" % (what, m.group(2),
                                                    m.group(1)))


def checkMismatch(display, data, work):
    url = makeCalc(data, "water-23", BAD_PROPS)
    out = os.path.join(work, "bad")
    result = runBuilder(display, url, out,
                        "mopanel first\nmogrid g1\nmopanel second\n"
                        "mogrid g2\n")
    log = result.log or ""
    what = "23-function ORCA output on a 24-function basis"
    dialogs = [l for l in log.splitlines() if "ECCE_TEST_DIALOG:" in l]
    if result.crashed:
        fail(what, "builder CRASHED (%s)" % result.signalName, log)
    elif ZERO_GRID in log:
        fail(what, "the zero-grid text was shown for a basis mismatch", log)
    elif len(dialogs) != 1:
        fail(what, "%d dialogs, want exactly one" % len(dialogs), log)
    elif MISMATCH not in log.replace("\n", " "):
        fail(what, "the dialog does not give both function counts", log)
    elif "9.0.0-alpha.7" not in log:
        fail(what, "the dialog does not say which ORCA decks to rerun", log)
    else:
        print("ok    %s: one dialog, both counts named" % what)


def shots(display, data, work, dest):
    os.makedirs(dest, exist_ok=True)
    url = makeCalc(data, "water-shot")
    out = os.path.join(work, "shot")
    full = os.path.join(out, "window.png")
    #  held open past the settle period, when the window is photographed
    runBuilder(display, url, out, "mopanel compute\n", shot=full)
    metrics = os.path.join(out, "metrics.txt")
    if os.path.exists(metrics):
        print(open(metrics).read())
    if os.path.exists(full):
        shutil.copy(full, os.path.join(dest, "builder-mo-panel.png"))
        print("shot  %s" % os.path.join(dest, "builder-mo-panel.png"))
    #  The mismatch dialog, left up while the window is photographed.
    url = makeCalc(data, "water-shot-23", BAD_PROPS)
    out = os.path.join(work, "shot-23")
    full = os.path.join(out, "window.png")
    runBuilder(display, url, out, "mopanel compute\n", shot=full,
               dialogSeconds="90")
    if os.path.exists(full):
        shutil.copy(full, os.path.join(dest, "mo-basis-mismatch.png"))
        print("shot  %s" % os.path.join(dest, "mo-basis-mismatch.png"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="save the Builder window here")
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="orca-mo-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    traceCheck()
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            if options.shots:
                shots(display, data, work, options.shots)
            else:
                checkGood(display, data, work)
                checkMismatch(display, data, work)
                for mode in ("classic", "stacked", "accordion", "detail"):
                    checkWidth(display, data, work, mode)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
