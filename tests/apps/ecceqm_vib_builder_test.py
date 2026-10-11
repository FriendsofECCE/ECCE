#!/usr/bin/env python3
"""The Builder on ECCE-QM Geometry and Vibration runs of water, headless, with
the tree's own binaries.

    tests/apps/ecceqm_vib_builder_test.py [--build build-cmake] [--png DIR]

The calculations are tests/ecceqm/fixtures/h2o-geometry, h2o-vibration and
h2o-geovib, made by tests/launch/ecceqm_test.py --export from real runs of the
bundled engine (B3LYP/6-31G*, local data mode).

  * Vibration, GeoVib: the Vibrational Frequencies panel opens with the IR
    spectrum pane; the sticks the canvas holds (ECCE_SPECTRUM_DUMP) are the
    three vibrations of the output file with its frequencies and intensities,
    nothing at the translations and rotations, and a click on a stick selects
    the mode, as for ORCA (tests/spectrum/capture.py).
  * Geometry, GeoVib: the Geometry Trace panel opens on the optimisation's
    steps; the window is photographed.

Screenshots go to --png (default: the run's state directory).  No synthetic
input: the Builder is driven by its own script hooks.  Exit status 77 (skip)
without Xvfb, import, python3-pil/numpy or the built binaries.
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

FIXTURES = os.path.join(REPO, "tests", "ecceqm", "fixtures")
PROJECT_META = bo.PROJECT_META
check = bo.check


def install(s, names):
    data = s.localData()
    project = os.path.join(data, "users", "local", "qm")
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    urls = {}
    for name in names:
        calc = os.path.join(project, name)
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(os.path.join(FIXTURES, name), calc, symlinks=True)
        urls[name] = "file://" + calc + "/"
    return urls


def outputVibrations(name):
    """(frequency, intensity) of the real vibrations, read straight from the
    stored engine output."""
    path = os.path.join(FIXTURES, name, "Outputs", "ecceqm.qmout")
    text = open(path).read()
    ntr = int(re.search(r"^vib_transrot (\d+)", text, re.M).group(1))
    rows = re.search(r"begin vibrations.*?\n(.*?)end vibrations", text, re.S).group(1).splitlines()
    return [(float(r.split()[1]), float(r.split()[2])) for r in rows][ntr:]


def readSticks(path):
    sticks, click = [], None
    for line in open(path):
        f = line.split()
        if f[:2] == ["stick", "ir"]:
            sticks.append((int(f[2]), float(f[3]), float(f[4])))
        elif f[:2] == ["click", "mode"]:
            click = line.strip()
    return sticks, click


def vibrationCase(s, display, env, wrappers, png, urls, name, click):
    say("Vibrational Frequencies panel, %s" % name)
    dump = os.path.join(s.state, "spectrum-%s.dump" % name)
    if os.path.exists(dump):
        os.unlink(dump)
    lines = ["wait 4000", "shot vib-" + name, "quit"]
    text, shots = bo.runScript(
        s, display, env, wrappers, ["-context", urls[name]], lines, png, name,
        {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies",
         "ECCE_SPECTRUM_DUMP": dump, "ECCE_SPECTRUM_CLICK": str(click)})
    if not check(os.path.exists(dump) and os.path.getsize(dump) > 0,
                 "the spectrum canvas dumped what it holds"):
        say(text[-2000:])
        return
    sticks, clicked = readSticks(dump)
    want = outputVibrations(name)
    check(len(sticks) == len(want),
          "%d IR sticks for %d vibrations (none for translations/rotations)" % (len(sticks), len(want)))
    for (mode, nu, inten), (wnu, wint) in zip(sticks, want):
        check(abs(nu - wnu) < 0.01 and abs(inten - wint) <= 1e-3 * max(1.0, wint),
              "mode %d: %.4f cm-1, %.4f km/mol; output file %.4f, %.4f" % (mode, nu, inten, wnu, wint))
    check(clicked is not None and "table row %d" % click in clicked,
          "a click on stick %d selects that mode: %s" % (click, clicked))
    check("crash" not in text.lower() and "segmentation" not in text.lower(), "no crash in the Builder log")
    check("vib-" + name in shots, "the window was photographed")


def traceCase(s, display, env, wrappers, png, urls, name):
    say("Geometry Trace panel, %s" % name)
    lines = ["wait 4000", "shot trace-" + name, "quit"]
    text, shots = bo.runScript(
        s, display, env, wrappers, ["-context", urls[name]], lines, png, "trace-" + name,
        {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Geometry Trace"})
    check("trace-" + name in shots, "the window was photographed")
    check("no property panel named" not in text, "the Builder offers a Geometry Trace panel")
    check("crash" not in text.lower() and "segmentation" not in text.lower(), "no crash in the Builder log")
    steps = len(re.findall(r"<step number=", open(os.path.join(FIXTURES, name, "Props", "TEVEC")).read()))
    check(steps >= 3, "the energy trace has %d steps" % steps)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    for name in ("h2o-geometry", "h2o-vibration", "h2o-geovib"):
        if not os.path.isdir(os.path.join(FIXTURES, name)):
            harness.skip("fixture %s is missing" % name)
    import xdisplay

    s = harness.Session(build, "ecceqmvib", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    say("screenshots: %s" % png)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        say("X display %s" % display.name)
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        urls = install(s, ("h2o-geometry", "h2o-vibration", "h2o-geovib"))
        vibrationCase(s, display, env, wrappers, png, urls, "h2o-vibration", 8)
        vibrationCase(s, display, env, wrappers, png, urls, "h2o-geovib", 8)
        traceCase(s, display, env, wrappers, png, urls, "h2o-geometry")
        traceCase(s, display, env, wrappers, png, urls, "h2o-geovib")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
