#!/usr/bin/env python3
"""Pull Force panel, headless, with the tree's own binaries.

    tests/apps/pull_panel_test.py [--build build-cmake] [--png DIR]

PULLTIME/PULLDIST/PULLFORCE of a calculation are written as property files
(a constant-rate ramp with a spring force of known size; the panel only
draws them, the parsing is tests/gromacs_md/pull_test.py), the Builder opens
the Pull Force panel and the script command "pullcsv" exports the CSV, whose
columns must equal the values written.  Exit status 77 (skip) without Xvfb,
import or binaries.
"""

import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import irc_panel_test as irc  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
N = 41
TIME = [0.1 * i for i in range(N)]
DIST = [2.0 + 0.03 * t for t in TIME]
FORCE = [5000 * 0.01 * (1 - 2.718281828 ** (-t)) for t in TIME]


def vector(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Pull Frame" rowLabels="%s" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, " ".join("%d-A" % (i + 1) for i in range(len(values))),
               len(values), units, "   ".join("%.6f" % v for v in values)))


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
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "pullpanel", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
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
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "calc-pull")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(irc.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(irc.HCN)
        for key, vals, units in (("PULLTIME", TIME, "ps"), ("PULLDIST", DIST, "nm"),
                                 ("PULLFORCE", FORCE, "kJ/mol/nm")):
            with open(os.path.join(props, key), "w") as f:
                f.write(vector(key, vals, units))
        csv = os.path.join(s.state, "pull.csv")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            ["wait 6000", "shot panel", "pullcsv %s" % csv, "quit"], png, "pullpanel",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Pull Force"})
        check("pullcsv points=%d ok" % N in text, "the panel opened with %d frames and exported" % N)
        if os.path.exists(csv):
            rows = [l.split(",") for l in open(csv).read().splitlines()
                    if l and l[0] != "#"][1:]
            check(len(rows) == N and all(
                abs(float(r[0]) - TIME[i]) < 1e-5 and abs(float(r[1]) - DIST[i]) < 1e-5
                and abs(float(r[2]) - FORCE[i]) < 1e-5 for i, r in enumerate(rows)),
                "the CSV holds the written time, distance and force")
        else:
            check(False, "the CSV exists")
        say("screenshot: %s" % shots.get("panel"))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
