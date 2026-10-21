#!/usr/bin/env python3
"""IRC Path panel (#417), headless, with the tree's own binaries.

    tests/apps/irc_panel_test.py [--build build-cmake] [--png DIR]

The path is read straight from the real Gaussian output
tests/parsers/fixtures/gaussian-16/hcn_irc.log (its "Summary of reaction path
following" table, by regex here, not by the parser) and written as property
files of a calculation.  The Builder opens the IRC Path panel and the script
command "ircpath" exports the CSV, whose energies must equal TS energy plus
the printed relative column and whose relative column must be zero at the TS.  Exit status 77 (skip) without Xvfb, import or
binaries.
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
BASE = os.path.join(HERE, "fixtures", "calc-water-freq-g16")
LOG = os.path.join(REPO, "tests", "parsers", "fixtures", "gaussian-16", "hcn_irc.log")
HARTREE_KJ = 2625.4996
HCN = """title: hcn
type: molecule
num_atoms: 3
atom_info: symbol cart
atom_list:
C -0.645876 0.087638 0.000000
N 0.522582 -0.147102 0.000000
H 0.429503 1.000360 0.000000
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
        6   1
        7   1
        1   1
num_bonds: 2
bond_list:
1 2 1.00000 1
1 3 1.00000 1
"""


def logPath():
    text = open(LOG).read()
    ts = float(re.search(r"relative to the TS energy of\s+(-?\d+\.\d+)", text).group(1))
    table = text.split("Summary of reaction path following")[1].split("Total number")[0]
    rows = re.findall(r"^\s+\d+\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s*$", table, re.M)
    return ([float(r[1]) for r in rows], [ts + float(r[0]) for r in rows], ts)


def vector(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="IRC Point" rowLabels="%s" rows="%d" '
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

    s = harness.Session(build, "ircpanel", {}, local=True, keep=args.keep)
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

    coord, energy, ts = logPath()
    assert len(coord) == 17
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
        calc = os.path.join(project, "calc-hcn-irc")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(HCN)
        with open(os.path.join(props, "IRCCOORD"), "w") as f:
            f.write(vector("IRCCOORD", coord, "sqrt(amu)*bohr"))
        with open(os.path.join(props, "IRCENERGY"), "w") as f:
            f.write(vector("IRCENERGY", energy, "Hartree"))

        kj = os.path.join(s.state, "irc-kj.csv")
        kcal = os.path.join(s.state, "irc-kcal.csv")
        lines = ["wait 6000", "shot panel", "ircpath %s" % kj,
                 "ircpath %s kcal" % kcal, "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "ircpanel",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "IRC Path"})
        check("ircpath points=17 ok" in text,
              "the panel opened with 17 IRC points and exported")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        if not (os.path.exists(kj) and os.path.exists(kcal)):
            say(text[-2000:])
            check(False, "both files exist")
            return 1
        say("screenshot: %s" % shots.get("panel"))

        lo = ts
        for path, scale in ((kj, HARTREE_KJ), (kcal, 627.5095)):
            rows = [l.split(",") for l in open(path).read().splitlines()
                    if l and not l.startswith("#")]
            head, rows = rows[0], rows[1:]
            check(len(rows) == 17, "%s: one row per IRC point" % head[3])
            check(all(abs(float(r[1]) - c) < 1e-4 for r, c in zip(rows, coord)),
                  "%s: coordinates equal the printed RxCoord" % head[3])
            check(all(abs(float(r[2]) - e) < 2e-6 for r, e in zip(rows, energy)),
                  "%s: absolute energies equal TS plus the printed column" % head[3])
            check(all(abs(float(r[3]) - (e - lo) * scale) < 1e-3
                      for r, e in zip(rows, energy)),
                  "%s: relative energies are to the TS" % head[3])
        ts_row = [l.split(",") for l in open(kj).read().splitlines()
                  if l and not l.startswith("#")][1:][8]
        check(abs(float(ts_row[3])) < 1e-6, "the TS row (coordinate 0) is the zero")
        check(any(l.startswith("# calculation: ")
                  for l in open(kj).read().splitlines()), "calculation named")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
