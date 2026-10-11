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

The Properties list highlights the panel that is shown (opened from
ECCE_OPEN_PANEL, the Properties menu or a click on the list), in the list +
detail and the classic layout.

The ESP colour bar (water, Gaussian 16, calc-water-vib): absent until an ESP
surface is drawn, then showing the range the surface's ramp was stretched
over, absent again once an MO is drawn instead.

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
WATER_G16 = os.path.join(FIXTURES, "calc-water-vib")
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


def makeCalc(data, name, props=None, source=GOOD):
    project = os.path.join(data, "users", "local", "orca-mo")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, name)
    shutil.copytree(source, calc, ignore=shutil.ignore_patterns(".DAV"))
    if source == WATER_G16:
        #  The fixture's properties live in the data server's WebDAV state;
        #  a local calculation reads the same facts from .ecce-meta.
        meta = open(os.path.join(GOOD, ".ecce-meta")).read()
        meta = meta.replace("\tORCA\n", "\tGaussian-16\n")
        meta = meta.replace("ecce:runtype\t\tEnergy", "ecce:runtype\t\tVibration")
        with open(os.path.join(calc, ".ecce-meta"), "w") as handle:
            handle.write(meta)
    if props:
        for f in os.listdir(props):
            shutil.copy(os.path.join(props, f), os.path.join(calc, "Props"))
    return "file://" + calc + "/"


def runBuilder(display, url, out, scene, shot=None, mode=None,
               dialogSeconds="1", extraEnv=None):
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
    env.update(extraEnv or {})
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


def readState(out, name):
    path = os.path.join(out, name + ".txt")
    if not os.path.exists(path):
        return None
    return dict(l.split(" ", 1) for l in open(path).read().splitlines()
                if " " in l)


def checkTree(display, data, work, mode):
    """The Properties list highlights the panel that is shown: the one
    ECCE_OPEN_PANEL opened, then the one a Properties menu choice opened, then
    the one a click on the list opened."""
    url = makeCalc(data, "tree-" + mode)
    out = os.path.join(work, "tree-" + mode)
    scene = ("hold 3\nptree opened\n"
             "property Mulliken Charges\nhold 3\nptree menu\n")
    if mode == "detail":      # the list is empty in the other layouts
        scene += "list MOs\nhold 3\nptree list\n"
    result = runBuilder(display, url, out, scene, mode=mode)
    what = "Properties list follows the shown panel, %s layout" % mode
    steps = [("opened", "MOs"), ("menu", "Mulliken Charges")]
    if mode == "detail":
        steps.append(("list", "MOs"))
    for name, want in steps:
        st = readState(out, name)
        if st is None:
            fail(what, "no state after the %s step" % name, result.log)
            return
        shown = [s for s in st.get("shown", "").split(",") if s]
        #  The list is only filled in the list + detail layout; elsewhere
        #  it must not keep a highlight of its own.
        wantTree = want if mode == "detail" else ""
        if st.get("tree", "") != wantTree:
            fail(what, "after %s the list highlights '%s', want '%s' "
                 "(shown: %s)" % (name, st.get("tree"), wantTree, shown))
            return
        if want not in shown or (mode == "detail" and shown != [want]):
            fail(what, "after %s the shown panels are %s, want %s"
                 % (name, shown, want))
            return
    print("ok    %s" % what)


def checkEspBar(display, data, work):
    """The ESP legend: hidden before an ESP surface is drawn and once an MO
    replaces it (choosing a type alone draws nothing, so the bar stays),
    on the ESP surface it carries the surface's colour range (fixed here by
    ECCE_ESP_RANGE, so the expected value does not come from the code that
    draws the bar) and says what the colours and the units are."""
    url = makeCalc(data, "esp-bar", source=WATER_G16)
    out = os.path.join(work, "esp-bar")
    scene = ("mobar before\nmofield Density (ESP)\nmobar chosen\n"
             "mopanel esp\nmobar drawn\nmofield MO\nmobar typed\n"
             "mopanel mo\nmobar mo\n")
    result = runBuilder(display, url, out, scene,
                        extraEnv={"ECCE_ESP_RANGE": "0.05"})
    what = "ESP colour bar"
    st = {n: readState(out, n) for n in
          ("before", "chosen", "drawn", "typed", "mo")}
    if any(v is None for v in st.values()):
        fail(what, "the scene did not finish: %s" % [n for n, v in st.items()
                                                      if v is None],
             result.log)
        return
    for name in ("before", "chosen", "mo"):
        if st[name].get("espbar_shown") != "1" and \
                st[name].get("espbar_shown") != "0":
            fail(what, "no bar state at %s" % name)
            return
        if st[name]["espbar_shown"] != "0":
            fail(what, "the bar is visible at '%s'; it belongs to a drawn "
                 "ESP surface only" % name)
            return
    d = st["drawn"]
    info = d.get("espbar_info", "")
    problems = []
    if d.get("espbar_shown") != "1":
        problems.append("not visible while the ESP surface is drawn")
    if d.get("espbar_min", "").strip() != "-0.0500" or \
            d.get("espbar_max", "").strip() != "+0.0500":
        problems.append("range %s .. %s, want -0.0500 .. +0.0500"
                        % (d.get("espbar_min"), d.get("espbar_max")))
    if d.get("espbar_colors") != "64":
        problems.append("%s colours, the ramp has 64" % d.get("espbar_colors"))
    for text in ("negative (electron-rich)", "positive (electron-poor)",
                 "Hartree/e", "kcal/mol", "e/bohr^3"):
        if text not in info:
            problems.append("legend lacks '%s'" % text)
    if "-31.4 to +31.4 kcal/mol" not in info:   # 0.05 * 627.5095
        problems.append("kcal/mol range is not 0.05 Hartree/e converted")
    if problems:
        fail(what, "; ".join(problems), info)
    else:
        print("ok    %s: shown only with the ESP surface, +/-0.05 Hartree/e"
              % what)

    #  Auto-scaled: the bar must show the range the surface was coloured
    #  over, which IsoSurfaceCmd logs independently of the bar.
    out = os.path.join(work, "esp-bar-auto")
    result = runBuilder(display, url, out,
                        "mofield Density (ESP)\nmopanel esp\nmobar auto\n",
                        extraEnv={"ECCE_DEBUG_ISO": "1"})
    what = "ESP colour bar, auto range"
    st = readState(out, "auto")
    used = re.findall(r"range \+/- (\S+)", result.log or "")
    if st is None or not used:
        fail(what, "no bar state or no range logged", result.log)
        return
    lo, hi = st["espbar_min"].strip(), st["espbar_max"].strip()
    want = float(used[-1])
    if abs(float(hi) - want) > 6e-5 or abs(float(lo) + want) > 6e-5 \
            or not want > 0:
        fail(what, "bar %s .. %s, the surface was coloured +/-%g"
             % (lo, hi, want))
    else:
        print("ok    %s: %s .. %s as the surface was coloured" % (what, lo,
                                                                   hi))


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
    parser.add_argument("--only", choices=("esp", "tree"),
                        help="run just the ESP bar or the tree checks")
    parser.add_argument("--keep", action="store_true",
                        help="keep the scratch directory for inspection")
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
            elif options.only == "esp":
                checkEspBar(display, data, work)
            elif options.only == "tree":
                for mode in ("detail", "classic"):
                    checkTree(display, data, work, mode)
            else:
                checkGood(display, data, work)
                checkMismatch(display, data, work)
                checkEspBar(display, data, work)
                for mode in ("detail", "classic"):
                    checkTree(display, data, work, mode)
                for mode in ("classic", "stacked", "accordion", "detail"):
                    checkWidth(display, data, work, mode)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    if options.keep:
        print("kept %s" % work)
    else:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
