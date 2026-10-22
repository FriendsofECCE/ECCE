#!/usr/bin/env python3
"""Relaxed Scan panel on an ECCE-QM scan (#450), headless.

    tests/apps/ecceqm_scan_panel_test.py [--build build-cmake] [--png DIR]

Runs the bundled ecce-qm on a relaxed torsion scan of H2O2 (HF/STO-3G, the
`scan` block ai.ecceqm writes), feeds its scan_table and scan_geometry blocks
through the real scripts/parsers/ecceqm.parse, stores what the parser returns
as the calculation's SCANCOORD, SCANENERGY and SCANGEOM, and lets the Builder
open the Relaxed Scan panel.  Checked: the panel exports the engine's own
coordinates and energies, a click on a point shows that point's optimised
structure, and "New calculation from this point" copies it.  Exit status 77
(skip) without Xvfb, import, ecce-qm or the binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import scan_panel_test as sp  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
DECK = os.path.join(REPO, "tests", "parsers", "fixtures", "ecceqm", "h2o2_scan.qmin")
PARSE = os.path.join(REPO, "scripts", "parsers", "ecceqm.parse")


def engine(build):
    for cand in ("src/qm/ecce-qm", "ecce-qm", "bin/ecce-qm"):
        path = os.path.join(build, cand)
        if os.access(path, os.X_OK):
            return path
    return None


def parsed(text):
    """The parser's 'key: ...' records as {key: {field: lines}}."""
    out = {}
    for rec in text.split("END\n"):
        m = re.match(r"\s*key: (\w+)\n", rec)
        if m:
            fields, cur = {}, None
            for line in rec.splitlines()[1:]:
                f = re.match(r"(\w+):\s*$", line)
                if f:
                    cur = f.group(1)
                    fields[cur] = []
                elif cur:
                    fields[cur].append(line)
            out.setdefault(m.group(1), []).append(fields)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import", "perl"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    qm = engine(build)
    if not qm:
        harness.skip("ecce-qm is not built in %s" % build)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "ecceqmscan", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    run = subprocess.run([qm, DECK], capture_output=True, text=True,
                         env=dict(os.environ, ECCE_QM_SERIAL="1"))
    check(run.returncode == 0, "ecce-qm ran the scan deck")
    out = run.stdout
    tables = re.findall(r"^begin scan_table\b.*?^end scan_table\n", out, re.S | re.M)
    frames_txt = re.findall(r"^begin scan_geometry\b.*?^end scan_geometry\n", out, re.S | re.M)
    check(len(tables) == 7 and len(frames_txt) == 7, "7 points in the engine output")
    feed = lambda t: parsed(subprocess.run(["perl", PARSE], input=t, capture_output=True,
                                           text=True).stdout)
    table = feed(tables[-1])
    coord = [float(v) for v in table["SCANCOORD"][0]["values"][0].split()]
    energy = [float(v) for v in table["SCANENERGY"][0]["values"][0].split()]
    frames = []
    for t in frames_txt:
        rec = feed(t)["SCANGEOM"][0]
        frames.append([[float(v) for v in row.split()] for row in rec["values"]])
    check(len(coord) == len(energy) == len(frames) == 7 and len(frames[0]) == 4,
          "the parser returned 7 coordinates, energies and 4-atom structures")
    say("scan: %s" % " ".join("%g:%.6f" % c for c in zip(coord, energy)))

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
        calc = os.path.join(project, "calc-h2o2-qmscan")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(sp.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(sp.H2O2)
        with open(os.path.join(props, "SCANCOORD"), "w") as f:
            f.write(sp.vector("SCANCOORD", coord, "Degree"))
        with open(os.path.join(props, "SCANENERGY"), "w") as f:
            f.write(sp.vector("SCANENERGY", energy, "Hartree"))
        with open(os.path.join(props, "SCANGEOM"), "w") as f:
            f.write(sp.tsvector(frames))

        kj = os.path.join(s.state, "scan-kj.csv")
        lines = ["wait 6000", "shot panel", "scancsv %s" % kj, "scanclick 3",
                 "scanrow 5", "scanstep next", "scancalc 3", "scanstatus", "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "ecceqmscan",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Relaxed Scan"})
        check("scancsv points=7 ok" in text, "the panel opened with 7 scan points and exported")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        if not os.path.exists(kj):
            say(text[-2000:])
            check(False, "the csv exists")
            return 1
        say("screenshot: %s" % shots.get("panel"))
        rows = [l.split(",") for l in open(kj).read().splitlines()
                if l and not l.startswith("#")][1:]
        check(len(rows) == 7, "one row per scan point")
        check(all(abs(float(r[1]) - c) < 1e-4 for r, c in zip(rows, coord)),
              "the panel's coordinates are the engine's")
        # scan_panel_test.vector writes five decimals
        check(all(abs(float(r[2]) - e) < 1e-5 for r, e in zip(rows, energy)),
              "the panel's energies are the engine's")
        clicks = dict((m[0], m) for m in re.findall(
            r"BUILDER: scanclick (\d+) frame=(\S+) atoms: (\S+) (\S+) (\S+) "
            r"(\S+) (\S+) (\S+)", text))
        got = clicks.get("3")
        check(got is not None and got[1] == "3", "a click on scan point 3 shows frame 3")
        if got:
            want = frames[3][0] + frames[3][1]
            check(all(abs(float(a) - b) < 2e-5 for a, b in zip(got[2:], want)),
                  "the viewer holds the engine's optimised structure of point 3")
        new = os.path.join(project, "calc-h2o2-qmscan_point_4")
        check(os.path.isdir(new), "New calculation from this point created %s" % os.path.basename(new))
        if os.path.isdir(new):
            coords = sp.mvmCoords(os.path.join(new, "Parameters", "chemsys.mvm"))
            check(len(coords) == 4 and all(abs(a - b) < 1e-4 for r, f in zip(coords, frames[3])
                                            for a, b in zip(r, f)),
                  "the new calculation starts from the point's structure")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
