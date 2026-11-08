#!/usr/bin/env python3
"""Builder: Edit > Distort Bonds on an octahedral SF6, undone (#619).

    tests/apps/builder_distort_test.py [--build DIR] [--keep] [--shots DIR]
Headless on an SF6 .car written by the test.  "cmd distort" is what the dialog
runs: every S-F distance grows by 0.1 along its bond, the Jahn-Teller preset
gives +0.1 on one trans pair and -0.05 on the rest, and Ctrl+Z restores the
coordinates exactly.

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



def sf6Car(path):
    r = 1.56
    atoms = [("S", 0, 0, 0), ("F", r, 0, 0), ("F", -r, 0, 0), ("F", 0, r, 0),
             ("F", 0, -r, 0), ("F", 0, 0, r), ("F", 0, 0, -r)]
    counts = {}
    with open(path, "w") as f:
        f.write("!BIOSYM archive 3\nPBC=OFF\nsf6\n!DATE Mon Oct 05 "
                "12:00:00 2026\n")
        for sym, x, y, z in atoms:
            n = counts[sym] = counts.get(sym, 0) + 1
            f.write("%-6s %14.9f %14.9f %14.9f XXXX 1      %-2s      %-2s"
                    "  0.000\n" % ("%s%d" % (sym, n), x, y, z, sym.lower(), sym))
        f.write("end\nend\n")


def dist(a, b):
    return math.sqrt(sum((p - q) ** 2 for p, q in zip(a, b)))


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

    s = harness.Session(build, "distort", {}, local=True, keep=args.keep)
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
        xyz = os.path.join(s.state, "sf6.car")
        sf6Car(xyz)
        lines = ["wait 3000", "coordsraw",
                 "cmd distort centre 1 dr 0.1", "coordsraw",
                 "key viewer undo", "coordsraw",
                 "cmd distort centre 1 jt 0.1 axis z", "coordsraw",
                 "key viewer undo",
                 # modeless dialog: two Applies, then Undo step by step
                 "select 1", "distortdlg open", "distortdlg open",
                 "distortdlg fill 0.1", "distortdlg apply", "coordsraw",
                 "distortdlg apply", "coordsraw",
                 "key viewer undo", "coordsraw", "key viewer undo", "coordsraw",
                 "select 2", "distortdlg state",
                 "select 1", "distortdlg state", "distortdlg close", "quit"]
        text = eb.runBuilder(s, env, wrappers, "distort", lines, shots,
                             structure=xyz)
        dumps = []
        for m in re.finditer(r"BUILDER: rawatom (\d+) (\S+) (\S+) (\S+)", text):
            if m.group(1) == "1":
                dumps.append([])
            dumps[-1].append(tuple(float(m.group(k)) for k in (2, 3, 4)))
        if not check(len(dumps) == 8 and all(len(d) == 7 for d in dumps),
                     "eight dumps of 7 atoms (%s)" % [len(d) for d in dumps]):
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
            return 1
        before, after, undone, jt, m1, m2, u1, u0 = dumps
        steps = [dist(after[i], after[0]) - dist(before[i], before[0])
                 for i in range(1, 7)]
        check(all(abs(x - 0.1) < 1e-6 for x in steps),
              "every S-F distance grew by 0.1 (%s)" % steps)
        check(dist(after[0], before[0]) < 1e-9, "the centre stayed")
        check(all(abs(after[i][k] - before[i][k] * (1.66 / 1.56)) < 1e-6
                  for i in range(1, 7) for k in range(3)),
              "bond directions unchanged")
        check(all(dist(a, b) < 1e-9 for a, b in zip(before, undone)),
              "undo restores every coordinate")
        j = sorted(round(dist(jt[i], jt[0]) - dist(before[i], before[0]), 6)
                   for i in range(1, 7))
        check(j == [-0.05] * 4 + [0.1] * 2,
              "Jahn-Teller: two +0.1, four -0.05 (%s)" % j)
        z = [i for i in range(1, 7) if abs(jt[i][2]) > 1]
        check(len(z) == 2, "the z pair is the axial one")
        st = [(m.group(1), m.group(2), m.group(3)) for m in re.finditer(
            r"BUILDER: distortdlg open=(\d) apply=(\d) note=\[(.*)\]", text)]
        check(len(st) == 8 and st[0][:2] == ("1", "1") and st[1][:2] == ("1", "1"),
              "dialog opens once, second open reuses it (%s)" % st[:2])
        g = lambda d, i: dist(d[i], d[0]) - dist(before[i], before[0])
        check(all(abs(g(m1, i) - 0.1) < 1e-6 and abs(g(m2, i) - 0.2) < 1e-6
                  for i in range(1, 7)), "two Applies add 0.1 each")
        check(all(abs(g(u1, i) - 0.1) < 1e-6 for i in range(1, 7)),
              "first undo removes only the second Apply")
        check(all(dist(a, b) < 1e-9 for a, b in zip(before, u0)),
              "second undo restores the start")
        check(st[5][:2] == ("1", "0") and "selected" in st[5][2],
              "deselected centre disables Apply with a note (%s)" % (st[5],))
        check(st[6][:2] == ("1", "1") and st[6][2] == "",
              "reselecting enables Apply again")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
