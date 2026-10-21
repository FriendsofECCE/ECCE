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


def logFrames():
    """The CURRENT STRUCTURE blocks of the log, in Gaussian's order: FORWARD
    points 1..n, then REVERSE 1..m (Cartesian rows, Angstrom)."""
    frames = []
    for block in open(LOG).read().split("CURRENT STRUCTURE")[1:]:
        block = block.split("CHANGE IN THE REACTION COORDINATE")[0]
        frames.append([[float(v) for v in m] for m in re.findall(
            r"^\s+\d+\s+\d+\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s*$",
            block, re.M)])
    return frames


def tsvector(frames):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
           'name="IRCGEOM" rowLabel="IRC Step" rowLabels="%s" rows="%d" '
           'units="Angstrom" vectorLabel="Coordinate" vectors="1">\n'
           % (" ".join(str(i) for i in range(len(frames[0]))), len(frames[0]))]
    for i, f in enumerate(frames):
        out.append('  <step number="%d">%s</step>\n' % (
            i + 1, "   ".join("%.6f" % v for row in f for v in row)))
    out.append("</tsvectable>\n")
    return "".join(out)


def vector(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="IRC Point" rowLabels="%s" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, " ".join("%d-A" % (i + 1) for i in range(len(values))),
               len(values), units, "   ".join("%.6f" % v for v in values)))


ORCA_DIR = os.path.join(REPO, "tests", "parsers", "fixtures", "orca")


def orcaPath():
    """IRC PATH SUMMARY energies of the real ORCA output and the structures of
    its _IRC_Full_trj.xyz (path order, TS in the middle), read by regex."""
    text = open(os.path.join(ORCA_DIR, "hcn_irc.out")).read()
    table = text.split("IRC PATH SUMMARY")[1].split("SUGGESTED CITATIONS")[0]
    energy = [float(m) for m in re.findall(
        r"^\s+\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s+\d+\.\d+\s+\d+\.\d+", table, re.M)]
    frames = []
    for block in open(os.path.join(ORCA_DIR, "hcn_irc_IRC_Full_trj.xyz")).read().split("\n3\n"):
        rows = re.findall(r"^\s*[A-Z][a-z]?\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s*$",
                          block, re.M)
        if rows:
            frames.append([[float(v) for v in r] for r in rows])
    return energy, frames


def orcaCase(s, display, env, wrappers, project, png):
    """The panel on an ORCA path: one structure per point, TS included."""
    energy, frames = orcaPath()
    assert len(energy) == 21 and len(frames) == 21
    coord = [float(i - 10) for i in range(21)]
    calc = os.path.join(project, "calc-orca-irc")
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(BASE, calc, symlinks=True)
    props = os.path.join(calc, "Props")
    for name in os.listdir(props):
        if name != "ESCF":
            os.unlink(os.path.join(props, name))
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
        f.write(HCN)
    with open(os.path.join(props, "IRCCOORD"), "w") as f:
        f.write(vector("IRCCOORD", coord, "Point"))
    with open(os.path.join(props, "IRCENERGY"), "w") as f:
        f.write(vector("IRCENERGY", energy, "Hartree"))
    with open(os.path.join(props, "IRCGEOM"), "w") as f:
        f.write(tsvector(frames))
    lines = ["wait 6000", "ircclick 10", "ircclick 3", "ircclick 17", "ircclick 0", "quit"]
    text, _ = bo.runScript(
        s, display, env, wrappers, ["-context", "file://" + calc + "/"],
        lines, png, "ircorca",
        {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "IRC Path"})
    clicks = dict((m[0], m) for m in re.findall(
        r"BUILDER: ircclick (\d+) frame=(\S+) atoms: (\S+) (\S+) (\S+) "
        r"(\S+) (\S+) (\S+)", text))
    for idx in (10, 3, 17, 0):
        got = clicks.get(str(idx))
        check(got is not None and got[1] == str(idx),
              "ORCA: click on point %d shows frame %d" % (idx, idx))
        if got:
            want = frames[idx][0] + frames[idx][1]
            check(all(abs(float(a) - b) < 2e-5 for a, b in zip(got[2:], want)),
                  "ORCA point %d: the viewer holds the trajectory's structure" % idx)


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

        frames = logFrames()
        assert len(frames) == 16 and all(len(f) == 3 for f in frames)
        with open(os.path.join(props, "IRCGEOM"), "w") as f:
            f.write(tsvector(frames))

        kj = os.path.join(s.state, "irc-kj.csv")
        kcal = os.path.join(s.state, "irc-kcal.csv")
        lines = ["wait 6000", "shot panel", "ircpath %s" % kj,
                 "ircpath %s kcal" % kcal, "ircclick 11", "ircclick 5", "ircclick 8", "quit"]
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
        # Summary index 8 is the TS.  Index 11 is forward point 3 (frame 2),
        # index 5 reverse point 3 (frame 8+3-1 = 10); the log is the oracle.
        clicks = dict((m[0], m) for m in re.findall(
            r"BUILDER: ircclick (\d+) frame=(\S+) atoms: (\S+) (\S+) (\S+) "
            r"(\S+) (\S+) (\S+)", text))
        for idx, frame in (("11", 2), ("5", 10)):
            got = clicks.get(idx)
            check(got is not None and got[1] == str(frame),
                  "click on point %s shows frame %d" % (idx, frame))
            if got:
                want = frames[frame][0] + frames[frame][1]
                check(all(abs(float(a) - b) < 2e-5
                          for a, b in zip(got[2:], want)),
                      "point %s: the viewer holds the log's structure" % idx)
        check(clicks.get("8", ("", "", ""))[1] == "none",
              "the TS point has no structure")
        check(any(l.startswith("# calculation: ")
                  for l in open(kj).read().splitlines()), "calculation named")
        orcaCase(s, display, env, wrappers, project, png)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
