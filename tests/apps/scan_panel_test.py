#!/usr/bin/env python3
"""Relaxed Scan panel (#393), headless, with the tree's own binaries.

    tests/apps/scan_panel_test.py [--build build-cmake] [--png DIR]

The scan values are those the parser extracts from the real Gaussian output
tests/parsers/fixtures/gaussian-16/h2o2_dihedral_scan.log (read from the
parser suite's golden file).  They are written as property files of a
calculation, the Builder opens the Relaxed Scan panel, and the script command
"scancsv" exports the CSV.  The file is read back and its energies compared
with the golden values.  Exit status 77 (skip) without Xvfb, import or
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
GOLDEN = os.path.join(REPO, "tests", "parsers", "expected", "g16-h2o2-scan.txt")
HARTREE_KJ = 2625.4996
H2O2 = """title: h2o2
type: molecule
num_atoms: 4
atom_info: symbol cart
atom_list:
O 0.000000 0.730000 0.000000
O 0.000000 -0.730000 0.000000
H 0.850000 0.900000 0.300000
H -0.850000 -0.900000 0.300000
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
        8   1
        8   1
        1   1
        1   1
num_bonds: 3
bond_list:
1 2 1.00000 1
1 3 1.00000 1
2 4 1.00000 1
"""


def golden(key):
    text = open(GOLDEN).read()
    m = re.search(r"key: %s\n\s+size: (\d+)\n\s+values: ([^\n]+)" % key, text)
    return [float(v) for v in m.group(2).split()]


def vector(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Scan Point" rowLabels="%s" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, " ".join("%d-A" % (i + 1) for i in range(len(values))),
               len(values), units, "   ".join("%.5f" % v for v in values)))


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

    s = harness.Session(build, "scanpanel", {}, local=True, keep=args.keep)
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

    coord, energy = golden("SCANCOORD"), golden("SCANENERGY")
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
        calc = os.path.join(project, "calc-h2o2-scan")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(H2O2)
        with open(os.path.join(props, "SCANCOORD"), "w") as f:
            f.write(vector("SCANCOORD", coord, "Degree"))
        with open(os.path.join(props, "SCANENERGY"), "w") as f:
            f.write(vector("SCANENERGY", energy, "Hartree"))

        kj = os.path.join(s.state, "scan-kj.csv")
        kcal = os.path.join(s.state, "scan-kcal.csv")
        lines = ["wait 6000", "shot panel", "scancsv %s" % kj,
                 "scancsv %s kcal" % kcal, "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "scanpanel",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Relaxed Scan"})
        check("scancsv points=7 ok" in text,
              "the panel opened with 7 scan points and exported")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        if not (os.path.exists(kj) and os.path.exists(kcal)):
            say(text[-2000:])
            check(False, "both files exist")
            return 1
        say("screenshot: %s" % shots.get("panel"))

        lo = min(energy)
        for path, scale in ((kj, HARTREE_KJ), (kcal, 627.5095)):
            rows = [l.split(",") for l in open(path).read().splitlines()
                    if l and not l.startswith("#")]
            head, rows = rows[0], rows[1:]
            check(len(rows) == 7, "%s: one row per scan point" % head[3])
            check(all(abs(float(r[1]) - c) < 1e-4 for r, c in zip(rows, coord)),
                  "%s: coordinates equal the parsed ones" % head[3])
            check(all(abs(float(r[2]) - e) < 1e-6 for r, e in zip(rows, energy)),
                  "%s: absolute energies equal the parsed ones" % head[3])
            check(all(abs(float(r[3]) - (e - lo) * scale) < 1e-3
                      for r, e in zip(rows, energy)),
                  "%s: relative energies are to the lowest point" % head[3])
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
