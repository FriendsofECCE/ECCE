#!/usr/bin/env python3
"""Build > Solvate (#1118): methanol in a water box with a 6 A margin, the
atom count the script reports, a saved XYZ with that many atoms, no water
oxygen within the cut-off of the solute, and undo back to the six atoms.
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
METHANOL = """6
methanol
C   -0.046   0.663   0.000
O   -0.046  -0.758   0.000
H   -1.086   0.975   0.000
H    0.437   1.077   0.890
H    0.437   1.077  -0.890
H    0.868  -1.065   0.000
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
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
        mol = os.path.join(s.state, "methanol.xyz")
        out = os.path.join(s.state, "solvated.xyz")
        with open(mol, "w") as h:
            h.write(METHANOL)

        text = eb.runBuilder(
            s, env, wrappers, "solvate",
            ["wait 3000", "info", "solvate solvent water216 margin 6 scale 1.0",
             "info", "expect lattice yes", "savexyz " + out,
             "key viewer undo", "wait 500", "info",
             "expect lattice no", "quit"],
            None, structure=mol)
        m = re.search(r"BUILDER: solvate molecules=(\d+) added=(\d+) atoms=(\d+) "
                      r"cell=(\S+) (\S+) (\S+) density=(\S+)", text)
        atoms = re.findall(r"BUILDER: atoms=(\d+) lattice=(\w+)", text)
        say("      | %s" % (m.group(0) if m else "no solvate line"))
        if check(m, "solvate reported its result"):
            nmol, added, total = int(m.group(1)), int(m.group(2)), int(m.group(3))
            cell = [float(m.group(i)) for i in (4, 5, 6)]
            check(added == 3 * nmol and total == 6 + added,
                  "6 solute atoms + 3 per water (%d molecules, %d atoms)" % (nmol, total))
            # extent + 2 x 6 A: about 14-15 A per side, ~100 waters
            check(all(13.0 < c < 16.0 for c in cell), "cell is extent + 12 A (%s)" % cell)
            check(60 < nmol < 140, "water count plausible (%d)" % nmol)
            check(len(atoms) >= 2 and atoms[1] == (str(total), "yes"),
                  "info after solvate: %d atoms and a lattice (%s)" % (total, atoms))
            check(atoms and atoms[-1] == ("6", "no"), "undo brings back methanol alone (%s)" % atoms)
            if check(os.path.exists(out), "savexyz wrote the solvated structure"):
                lines = open(out).read().splitlines()
                check(int(lines[0]) == total, "saved XYZ has %s atoms" % lines[0])
                xyz = [(l.split()[0], [float(v) for v in l.split()[1:4]]) for l in lines[2:2 + int(lines[0])]]
                solute, water = xyz[:6], xyz[6:]
                close = min(math.dist(p, q) for _, p in solute for e, q in water if e == "O")
                check(close > 2.0, "no water oxygen within 2 A of methanol (closest %.2f)" % close)
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
