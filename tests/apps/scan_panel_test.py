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


ALLXYZ = os.path.join(REPO, "tests", "parsers", "fixtures", "orca",
                      "h2o2_dihedral_scan.allxyz")


def allxyzFrames():
    """The 7 scan structures of the real ORCA .allxyz, read by regex."""
    frames = []
    for block in open(ALLXYZ).read().split(">"):
        rows = re.findall(r"^\s*[A-Z][a-z]?\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)"
                          r"\s+(-?\d+\.\d+)\s*$", block, re.M)
        if rows:
            frames.append([[float(v) for v in r] for r in rows])
    return frames


def tsvector(frames):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
           'name="SCANGEOM" rowLabel="Scan Point" rowLabels="%s" rows="%d" '
           'units="Angstrom" vectorLabel="Coordinate" vectors="1">\n'
           % (" ".join(str(i) for i in range(len(frames[0]))), len(frames[0]))]
    for i, f in enumerate(frames):
        out.append('  <step number="%d">%s</step>\n' % (
            i + 1, "   ".join("%.6f" % v for row in f for v in row)))
    out.append("</tsvectable>\n")
    return "".join(out)


def mvmCoords(path):
    """Cartesian rows of the atom_list of a chemsys.mvm."""
    text = open(path).read().split("atom_list:")[1].split("attr_list:")[0]
    return [[float(v) for v in l.split()[1:4]] for l in text.strip().splitlines()]


def metaValue(calc, key):
    for l in open(os.path.join(calc, ".ecce-meta")).read().splitlines():
        f = l.split("\t")
        if len(f) >= 4 and f[1].endswith(":" + key):
            return f[3]
    return None


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
        frames = allxyzFrames()
        assert len(frames) == 7 and all(len(f) == 4 for f in frames)
        with open(os.path.join(props, "SCANGEOM"), "w") as f:
            f.write(tsvector(frames))

        kj = os.path.join(s.state, "scan-kj.csv")
        kcal = os.path.join(s.state, "scan-kcal.csv")
        lines = ["wait 6000", "shot panel", "scancsv %s" % kj,
                 "scancsv %s kcal" % kcal, "scanclick 3", "scanclick 5",
                 "scancalc 3", "quit"]
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

        # Point index 3 is frame 3 (0-based) of the structures.
        clicks = dict((m[0], m) for m in re.findall(
            r"BUILDER: scanclick (\d+) frame=(\S+) atoms: (\S+) (\S+) (\S+) "
            r"(\S+) (\S+) (\S+)", text))
        for idx in (3, 5):
            got = clicks.get(str(idx))
            check(got is not None and got[1] == str(idx),
                  "click on scan point %d shows frame %d" % (idx, idx))
            if got:
                want = frames[idx][0] + frames[idx][1]
                check(all(abs(float(a) - b) < 2e-5 for a, b in zip(got[2:], want)),
                      "scan point %d: the viewer holds the ORCA structure" % idx)
        new = os.path.join(project, "calc-h2o2-scan_point_4")
        check("scancalc 3 url=" in text and os.path.isdir(new),
              "scancalc 3 (the 4th point, 0-based index 3) created calc-h2o2-scan_point_4")
        if os.path.isdir(new):
            for key in ("application", "theory", "runtype", "basisSetDocument"):
                check(metaValue(calc, key) is not None
                      and metaValue(new, key) == metaValue(calc, key),
                      "new calculation keeps %s (%s)" % (key, metaValue(new, key)))
            check(metaValue(new, "state") == "Created",
                  "new calculation is in the created state (%s)" % metaValue(new, "state"))
            got = mvmCoords(os.path.join(new, "Parameters", "chemsys.mvm"))
            check(len(got) == 4 and all(abs(a - b) < 1e-4 for r, f in zip(got, frames[3])
                                         for a, b in zip(r, f)),
                  "new calculation's coordinates equal SCANGEOM frame 3")
            check(not os.path.exists(os.path.join(new, "Props", "SCANGEOM")),
                  "the copy carries no results")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
