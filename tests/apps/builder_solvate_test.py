#!/usr/bin/env python3
"""Build > Solvate (#1118): methanol from SMILES in a water box with a 6 A margin, the
atom count the script reports, a saved XYZ with that many atoms, no water
oxygen within 2 A of the solute, and undo back to the unsolvated structure.
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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.environ.get(
        "ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake")))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "solvate", {}, local=True, keep=args.keep)
    for exe in bo.GUI + ("autosym", "cleansym", "genmollat"):
        if os.path.exists(os.path.join(build, exe)):
            harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    text = ""
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        out = os.path.join(s.state, "solvated.xyz")

        text = eb.runBuilder(
            s, env, wrappers, "solvate",
            ["wait 3000", "smiles CO", "wait 500", "info", "solvate solvent water216 margin 6 scale 1.0",
             "info", "expect lattice yes", "savexyz " + out,
             "key viewer undo", "wait 500", "info",
             "expect lattice no", "quit"],
            None, structure=None)
        m = re.search(r"BUILDER: solvate molecules=(\d+) added=(\d+) atoms=(\d+) "
                      r"cell=(\S+) (\S+) (\S+) density=(\S+)", text)
        atoms = re.findall(r"BUILDER: atoms=(\d+) lattice=(\w+)", text)
        say("      | %s" % (m.group(0) if m else "no solvate line"))
        if check(m, "solvate reported its result"):
            nmol, added, total = int(m.group(1)), int(m.group(2)), int(m.group(3))
            cell = [float(m.group(i)) for i in (4, 5, 6)]
            solute = int(atoms[0][0]) if atoms else -1
            check(solute > 0 and added == 3 * nmol and total == solute + added,
                  "solute atoms + 3 per water (%d before, %d molecules, %d atoms)"
                  % (solute, nmol, total))
            # each side is the structure's extent + 2 x 6 A
            check(all(12.0 < c < 40.0 for c in cell), "cell is extent + 12 A (%s)" % cell)
            check(nmol > 30, "water count plausible (%d)" % nmol)
            check(len(atoms) >= 2 and atoms[1] == (str(total), "yes"),
                  "info after solvate: %d atoms and a lattice (%s)" % (total, atoms))
            check(len(atoms) >= 3 and atoms[-1] == (atoms[0][0], "no"),
                  "undo brings back the structure without solvent (%s)" % atoms)
            if check(os.path.exists(out), "savexyz wrote the solvated structure"):
                lines = open(out).read().splitlines()
                check(int(lines[0]) == total, "saved XYZ has %s atoms" % lines[0])
                xyz = [(l.split()[0], [float(v) for v in l.split()[1:4]]) for l in lines[2:2 + int(lines[0])]]
                solute_xyz, water = xyz[:solute], xyz[solute:]
                close = min(math.dist(p, q) for _, p in solute_xyz for e, q in water if e == "O")
                check(close > 2.0, "no water oxygen within 2 A of the solute (closest %.2f)" % close)
        check("FAIL" not in "\n".join(l for l in text.splitlines()
                                       if "ECCE_BUILDER_SCRIPT" in l),
              "no script step failed")
        if eb.failures:
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
