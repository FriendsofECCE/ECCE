#!/usr/bin/env python3
"""Edit > Take Coordinates From..., through the Builder scene hook, and undo (#620).

The open structure is an SF6 .car; the other structure is an .xyz of the same
atoms, rotated and shifted, with two fluorines moved.  Taking those two atoms
with Align on gives the open structure back except for them; a different
element order is refused; undo restores every coordinate.

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

import builder_distort_test as bd  # noqa: E402
import builder_edit_bugs_test as eb  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = eb.check

ATOMS = [("S", 0, 0, 0), ("F", 1.56, 0, 0), ("F", -1.56, 0, 0),
         ("F", 0, 1.56, 0), ("F", 0, -1.56, 0), ("F", 0, 0, 1.56),
         ("F", 0, 0, -1.56)]


def rot(p):
    """A proper rotation (90 degrees about z, then 90 about x) and a shift."""
    x, y, z = p
    x, y = -y, x
    y, z = -z, y
    return (x + 3.0, y - 2.0, z + 5.0)


def writeXyz(path, atoms, moved):
    with open(path, "w") as f:
        f.write("%d\nother\n" % len(atoms))
        for i, (sym, x, y, z) in enumerate(atoms):
            px, py, pz = rot((x, y, z))
            if i in moved:  # moved in the other structure's own frame
                px += 0.3
                py -= 0.2
            f.write("%s %.12f %.12f %.12f\n" % (sym, px, py, pz))


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

    s = harness.Session(build, "takecoords", {}, local=True, keep=args.keep)
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
        car = os.path.join(s.state, "sf6.car")
        bd.sf6Car(car)
        good = os.path.join(s.state, "other.xyz")
        writeXyz(good, ATOMS, {2, 5})   # atoms 3 and 6 (1-based)
        swapped = os.path.join(s.state, "swapped.xyz")
        writeXyz(swapped, [ATOMS[0], ("O",) + ATOMS[1][1:]] + ATOMS[2:], set())
        lines = ["wait 3000", "coordsraw",
                 "takefrom %s 3,6 align" % good, "coordsraw",
                 "key viewer undo", "coordsraw",
                 "takefrom %s 3,6 align" % swapped, "coordsraw",
                 "takefrom %s 1-5 align" % good, "quit"]
        text = eb.runBuilder(s, env, wrappers, "takecoords", lines, None,
                             structure=car)
        dumps = []
        for m in re.finditer(r"BUILDER: rawatom (\d+) (\S+) (\S+) (\S+)", text):
            if m.group(1) == "1":
                dumps.append([])
            dumps[-1].append(tuple(float(m.group(k)) for k in (2, 3, 4)))
        if not check(len(dumps) == 4 and all(len(d) == 7 for d in dumps),
                     "four dumps of 7 atoms (%s)" % [len(d) for d in dumps]):
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
            return 1
        before, after, undone, refused = dumps
        res = re.findall(r"BUILDER: takefrom (\w+) rmsd=(\S+)", text)
        check([r[0] for r in res] == ["ok", "refused", "refused"],
              "takefrom results ok, refused, refused (%s)" % res)
        check(res and float(res[0][1]) < 1e-6, "the fit of the other atoms is exact")
        d = [bd.dist(a, b) for a, b in zip(before, after)]
        check(all(d[i] < 1e-6 for i in (0, 1, 3, 4, 6)),
              "atoms outside the set are unchanged (%s)" % d)
        check(d[2] > 0.1 and d[5] > 0.1, "the two chosen atoms moved")
        # In A's frame the chosen atoms sit 0.3 / -0.2 off, as in B's frame
        # rotated back: undo the shift, then the two rotations.
        ok = True
        for i in (2, 5):
            px, py, pz = rot(before[i])
            px += 0.3
            py -= 0.2
            x, y, z = px - 3.0, py + 2.0, pz - 5.0
            y, z = z, -y          # inverse of (y, z) = (-z, y)
            x, y = y, -x          # inverse of (x, y) = (-y, x)
            ok = ok and bd.dist((x, y, z), after[i]) < 1e-6
        check(ok, "the chosen atoms equal the other structure, aligned")
        check(all(bd.dist(a, b) < 1e-9 for a, b in zip(before, undone)),
              "undo restores every coordinate")
        check(all(bd.dist(a, b) < 1e-9 for a, b in zip(before, refused)),
              "a different element order changes nothing")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
