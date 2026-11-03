#!/usr/bin/env python3
"""Two hydrogens 4 A apart are drawn when the calculation opens (#832).

    tests/apps/h2_far_atoms_test.py [--build build-cmake] [--png DIR]

Opens the stored H2 example as is (0.74 A), with its geometry moved to
4.0 A, and as a bond scan from 4.0 A down to 0.6 A that stores its last
point; the scan also with the Relaxed Scan and Geometry Trace panels open,
which show the 4.0 A start.  Both atoms must be inside the view on opening,
and Select All (which re-fits the camera) must not change the framing.
Exit 77 (skip) without Xvfb.
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
H2 = os.path.join(REPO, "data", "client", "examples", "genchem",
                  "molecular-orbitals-of-diatomics", "ecceqm-H2", "stored", "H2")
HEAD = ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n')


def setDistance(calc, r):
    path = os.path.join(calc, "Parameters", "chemsys.mvm")
    with open(path) as handle:
        text = handle.read()
    text = re.sub(r"atom_list: \n.*\n.*\n",
                  "atom_list: \nH 0.00000 0.00000 0.00000\n"
                  "H %.5f 0.00000 0.00000\n" % r, text)
    with open(path, "w") as handle:
        handle.write(text)


def trace(name, label, rs):
    steps = "".join('  <step number="%d">0.0 0.0 0.0 %.4f 0.0 0.0</step>\n'
                    % (i + 1, r) for i, r in enumerate(rs))
    return (HEAD + '<tsvectable columnLabel="Atom" columnLabels="X Y Z" '
            'columns="3" name="%s" rowLabel="%s" rowLabels="1-H 2-H" '
            'rows="2" units="Angstrom" vectorLabel="Coordinate" vectors="1">\n'
            '\n%s\n</tsvectable>\n' % (name, label, steps))


def vector(name, units, values):
    return (HEAD + '<vector name="%s" rowLabel="Scan Point" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, len(values), units, "   ".join("%.5f" % v for v in values)))


def makeCalc(project, name, rs, scan, final):
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(H2, calc, symlinks=True)
    setDistance(calc, rs[-1] if final else rs[0])
    props = os.path.join(calc, "Props")
    with open(os.path.join(props, "GEOMTRACE"), "w") as handle:
        handle.write(trace("GEOMTRACE", "Geometry Step", rs))
    if scan:
        with open(os.path.join(props, "SCANGEOM"), "w") as handle:
            handle.write(trace("SCANGEOM", "Scan Step", rs))
        with open(os.path.join(props, "SCANCOORD"), "w") as handle:
            handle.write(vector("SCANCOORD", "Angstrom", rs))
        with open(os.path.join(props, "SCANENERGY"), "w") as handle:
            handle.write(vector("SCANENERGY", "Hartree",
                                [-1.0 - 0.1 / r for r in rs]))
    return calc


def atomPixels(path):
    """Pixels inside the viewer that are neither its background nor grey."""
    import numpy
    from PIL import Image
    img = Image.open(path)
    box = bo.viewerBox(img)
    if not box:
        return 0
    px = numpy.asarray(img.convert("RGB").crop(box)).astype(int)
    bright = px.sum(axis=2) > 150
    return int(bright.sum())


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb") or not shutil.which("import"):
        harness.skip("Xvfb/import missing")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "h2far", {}, local=True, keep=args.keep)
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
    scan = [round(4.0 - 0.1 * i, 2) for i in range(35)]
    cases = [("h2_074", [0.74], False, False, None),
             ("h2_400", [4.0], False, False, None),
             ("h2_scan", scan, True, True, None),
             ("h2_scan_panel", scan, True, True, "Relaxed Scan"),
             ("h2_scan_trace", scan, True, True, "Geometry Trace")]
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
        for name, rs, scan, final, panel in cases:
            calc = makeCalc(project, name, rs, scan, final)
            extra = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": panel} \
                if panel else None
            argv = ["-context", "file://" + calc + "/"]
            run = ["wait 6000", "info", "inview", "shot open",
                   "key viewer selectall", "wait 500", "select 0", "wait 500",
                   "info", "shot after", "quit"]
            text, shots = bo.runScript(s, display, env, wrappers, argv, run,
                                       png, name, extra)
            inview = re.findall(r"inview outside=(\d)", text)
            check(inview[:1] == ["0"], "%s: both atoms in view on opening" % name)
            if "open" not in shots or "after" not in shots:
                check(False, "%s: both pictures taken" % name)
                continue
            before = atomPixels(shots["open"])
            after = atomPixels(shots["after"])
            say("%s: %d atom pixels on opening, %d after Select All"
                % (name, before, after))
            check(before > 0, "%s: atoms drawn on opening" % name)
            check(abs(before - after) < 0.2 * max(before, after),
                  "%s: Select All does not change the framing" % name)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
