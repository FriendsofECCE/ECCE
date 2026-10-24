#!/usr/bin/env python3
"""The Builder's Geometry Trace and Geometry Step Plots for a GROMACS run (#480).

    tests/apps/md_plots_test.py [--build build-cmake] [--png DIR] [--tag TAG]

A real 50 ps GROMACS NPT run (tests/parsers/fixtures/gromacs/water_npt.log,
gmx 2025.2) goes through the real parse scripts and is stored as a
calculation's Props.  The Builder (this tree's binaries) draws both panels
(scene command `plotshot`, which reports each canvas on stderr).  Expected:
the x axis is simulation time in ps taken from the log's "Step Time" rows
(read here by regular expression), the y axis has its unit, the title is a
readable name, and a thin "moving average (N ps window)" curve.

Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or binaries.
"""
import argparse
import glob
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
import md_summary_test as ms  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
LOG = os.path.join(REPO, "tests", "parsers", "fixtures", "gromacs", "water_npt.log")
UNITS = ("kJ/mol", "bar", "K", "kg/m\u00b3", "nm\u00b3")


def logTimes(text):
    """Times (ps) of the log's steps: the value row under "Step  Time"."""
    return [float(m.group(1)) for m in
            re.finditer(r"^\s*Step\s+Time\s*\n\s*\d+\s+(\S+)\s*$", text, re.M)]


def plotLines(out, name):
    plot = re.search(r"PLOTSHOT-PLOT %s title='([^']*)' x='([^']*)' y='([^']*)' "
                     r"curves=(\d+)" % re.escape(name), out)
    curves = {}
    for m in re.finditer(r"PLOTSHOT-CURVE %s (\d+) name='([^']*)' n=(\d+) "
                         r"lastx=(\S+) lasty=(\S+) meany=(\S+)" % re.escape(name), out):
        curves[int(m.group(1))] = (m.group(2), int(m.group(3)), float(m.group(4)),
                                   float(m.group(5)), float(m.group(6)))
    return plot, curves


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--tag", default="after")
    ap.add_argument("--qm", action="store_true",
                    help="pictures of a QM optimisation (the fixture as it is)")
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
    import xdisplay

    ms.TSVEC.update(("TIMEVEC", "MDDENSVEC", "MDVOLVEC"))
    props = ms.parsedProps(LOG)
    text = open(LOG).read()
    times = logTimes(text)
    check("TIMEVEC" in props and len(props["TIMEVEC"][1]) == len(times),
          "the parse scripts give one time per log step (%d)" % len(times))
    work = tempfile.mkdtemp(prefix="mdplots-")
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"),
                    os.path.join(work, "calc-md"), symlinks=True)
    if not args.qm:
        ms.writeProps(os.path.join(work, "calc-md"), props)
        #  writeProps replaces Props; the trace is what the Geometry Trace plays.
        shutil.copy(os.path.join(HERE, "fixtures", "calc-water-opt", "Props", "GEOMTRACE"),
                    os.path.join(work, "calc-md", "Props", "GEOMTRACE"))

    s = harness.Session(build, "mdplots", {}, local=True, keep=args.keep)
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
        url = eg.install(s, "calc-md", work)
        tag = args.tag
        if args.qm:
            tag = "qm-" + tag
        out = ""
        for suffix, panel in (("trace", "Geometry Trace"),
                              ("steps", "Geometry Step Plots")):
            scene = os.path.join(work, "scene-" + suffix)
            with open(scene, "w") as handle:
                handle.write("plotshot %s-%s 880 520 %s\nhold 1\n"
                             % (tag, suffix, panel))
            text, _shots = bo.runScript(
                s, display, env, wrappers, ["-context", url],
                ["wait 25000", "quit"], png, "mdplots-" + suffix,
                {"ECCE_PANEL_MODE": "detail", "ECCE_VIEWER_SCENE": scene,
                 "ECCE_VIEWER_SCENE_OUT": os.path.join(s.state, "mdplots-" + suffix)})
            out += text
        for pic in glob.glob(os.path.join(s.state, "mdplots-*", "*.png")):
            shutil.copy(pic, png)
        say("pictures: %s" % png)
        check("crash" not in out.lower(), "no crash in the Builder log")
        if tag != "after":
            return 0
        for name in ("after-trace", "after-steps"):
            plot, curves = plotLines(out, name)
            if not check(plot is not None and len(curves) == 2,
                         "%s: a plot with two curves" % name):
                say(out[-2000:])
                continue
            title, xl, yl = plot.group(1), plot.group(2), plot.group(3)
            check(title and not title.endswith("Vector"),
                  "%s: readable title %r" % (name, title))
            check(xl == "Time (ps)", "%s: x axis is %r" % (name, xl))
            check(yl in UNITS, "%s: y axis unit %r" % (name, yl))
            (n0, k0, x0, y0, mean0), (n1, k1, x1, y1, _m) = curves[0], curves[1]
            check(re.match(r"moving average \(\S+ ps window\)$", n1) and k0 == k1 == len(times),
                  "%s: moving average beside the series (%s, %d points)" % (name, n1, k1))
            check(abs(x0 - times[-1]) < 1e-6 and abs(x1 - times[-1]) < 1e-6,
                  "%s: last x is the log's last time %g (got %g)" % (name, times[-1], x0))
            check(y1 == y1, "%s: moving average ends on a number (%g)" % (name, y1))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
        shutil.rmtree(work, ignore_errors=True)
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
