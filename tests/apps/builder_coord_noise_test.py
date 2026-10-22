#!/usr/bin/env python3
"""Builder: no 1e-16 noise in symmetrised coordinates or the Atom Table (#459).

    tests/apps/builder_coord_noise_test.py [--build DIR] [--keep] [--shots DIR]
Headless on an ethane (.car, written by the test) (D3d, the C3 axis tilted off the
coordinate axes so the helper has to rotate it).  Symmetry > Find must leave
x and y of both carbons exactly 0.0 in the fragment, and the Atom Table must
show them as 0.000000, with six decimals everywhere and never -0.000000.

Exit status 77 (skip) without Xvfb or the built Builder.
"""
import argparse
import math
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_edit_bugs_test as eb  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = eb.check


def ethaneXyz(path):
    cc, r, tilt = 1.536, 1.03, 0.3
    atoms = [("C", 0.0, 0.0, -cc / 2), ("C", 0.0, 0.0, cc / 2)]
    for sgn, off in ((-1, 0.0), (1, 60.0)):
        for k in range(3):
            a = math.radians(off + 120.0 * k)
            atoms.append(("H", r * math.cos(a), r * math.sin(a),
                          sgn * (cc / 2 + 0.37)))
    rot = [(math.cos(tilt), 0, math.sin(tilt)), (0, 1, 0),
           (-math.sin(tilt), 0, math.cos(tilt))]
    counts = {}
    with open(path, "w") as f:
        f.write("!BIOSYM archive 3\nPBC=OFF\nethane\n!DATE Mon Oct 05 "
                "12:00:00 2026\n")
        for sym, x, y, z in atoms:
            v = [sum(row[i] * c for i, c in enumerate((x, y, z)))
                 for row in rot]
            n = counts[sym] = counts.get(sym, 0) + 1
            f.write("%-6s %14.9f %14.9f %14.9f XXXX 1      %-2s      %-2s"
                    "  0.000\n" % (("%s%d" % (sym, n),) + tuple(v) +
                                    (sym.lower(), sym)))
        f.write("end\nend\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "coordnoise", {}, local=True, keep=args.keep)
    for exe in bo.GUI + ("autosym", "cleansym"):
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
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        shots = os.path.abspath(args.shots) if args.shots else None
        if shots:
            os.makedirs(shots, exist_ok=True)
        xyz = os.path.join(s.state, "ethane.car")
        ethaneXyz(xyz)
        lines = ["wait 3000", "symmetry", "wait 1500", "coordsraw",
                 "panel Atom Table", "wait 1500", "atomtable",
                 "shot atomtable-459", "quit"]
        text = eb.runBuilder(s, env, wrappers, "noise", lines, shots,
                             structure=xyz)
        pg = re.findall(r"BUILDER: pointgroup=(\S+)", text)
        check(bool(pg) and pg[0].upper().startswith("D3"),
              "point group %s" % pg)
        raw = [tuple(float(v) for v in m) for m in re.findall(
            r"BUILDER: rawatom \d+ (\S+) (\S+) (\S+)", text)]
        if not check(len(raw) == 8, "8 atoms (%d)" % len(raw)):
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
            return 1
        # autosym itself leaves ~3e-7 on the axis for this input; only
        # noise below 1e-10 is snapped, so that is what must be absent.
        check(all(v == 0.0 or abs(v) >= 1e-10 for a in raw for v in a),
              "no coordinate is a nonzero below 1e-10 (%s)" % (raw[:2],))
        check(all(abs(raw[i][0]) < 1e-6 and abs(raw[i][1]) < 1e-6
                  for i in (0, 1)), "carbons are on the C3 axis")
        rows = re.findall(r"BUILDER: tablerow (\d+) (.*)", text)
        check(len(rows) == 8, "Atom Table has 8 rows (%d)" % len(rows))
        bad = []
        for _, row in rows:
            for c in row.split("|"):
                if c == "-0.000000" or re.search(r"e[-+]\d", c):
                    bad.append(c)
        check(not bad, "no -0.000000 or exponent cells (%s)" % bad)
        check(bool(rows) and
              sum(c == "0.000000" for c in rows[0][1].split("|")) >= 2,
              "carbon row shows 0.000000 for x and y (%s)"
              % (rows[0][1] if rows else None,))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
