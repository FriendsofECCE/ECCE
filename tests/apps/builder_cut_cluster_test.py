#!/usr/bin/env python3
"""Build > Cut Cluster (#1033): a 5 A cluster of alpha-quartz around one Si
(H caps, then OH caps after an undo), and whole molecules out of a
CO2 molecular crystal.  Exit status 77 (skip) without Xvfb or the built Builder.
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
RADIUS = {"Si": 1.11, "O": 0.66, "H": 0.31, "C": 0.77}


def blocks(text):
    """The atoms of each cluster the log reports, as [(symbol, (x, y, z))]."""
    out = []
    for part in text.split("BUILDER: cluster atoms=")[1:]:
        out.append([(m[0], tuple(float(v) for v in m[1:])) for m in re.findall(
            r"BUILDER: clusteratom (\S+) (\S+) (\S+) (\S+)", part)])
    return out


def neighbours(atoms):
    """Bonded partners of each atom: radii sum plus 0.4 A, as the cut uses."""
    nb = [[] for _ in atoms]
    for i, (si, pi) in enumerate(atoms):
        for j, (sj, pj) in enumerate(atoms):
            if i != j and math.dist(pi, pj) <= RADIUS[si] + RADIUS[sj] + 0.4:
                nb[i].append(j)
    return nb


def quartz_checks(atoms, label, radius):
    nb = neighbours(atoms)
    sis = [i for i, a in enumerate(atoms) if a[0] == "Si"]
    check(sis, "%s: has silicon" % label)
    check(all(len(nb[i]) == 4 for i in sis),
          "%s: every Si has 4 neighbours (%s)" % (label, [len(nb[i]) for i in sis]))
    hs = [i for i, a in enumerate(atoms) if a[0] == "H"]
    check(hs, "%s: has capping H" % label)
    check(all(len(nb[i]) == 1 for i in hs),
          "%s: every H has one neighbour (%s)" % (label, sorted(set(len(nb[i]) for i in hs))))
    # Some Si is the centre: every real atom within radius of it, caps one bond further.
    fit = [c for c in sis
           if all(math.dist(atoms[c][1], p) <= radius + 0.01 for s, p in atoms
                  if s != "H")
           and all(math.dist(atoms[c][1], p) <= radius + 2.0 for s, p in atoms)]
    check(fit, "%s: atoms lie within %g A (+ one bond) of one Si" % (label, radius))


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

    s = harness.Session(build, "cutcluster", {}, local=True, keep=args.keep)
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
        fixtures = os.path.join(REPO, "tests", "fixtures", "cif")

        text = eb.runBuilder(
            s, env, wrappers, "quartz",
            ["wait 3000", "selectelem Si", "cutcluster 5 cap H", "info",
             "expect lattice no", "key viewer undo", "wait 500", "info",
             "expect lattice yes", "selectelem Si",
             "cutcluster 5 whole units cap OH", "expect lattice no", "quit"],
            None, structure=os.path.join(fixtures, "quartz.cif"))
        found = blocks(text)
        if check(len(found) == 2, "quartz: two clusters reported (%d)" % len(found)):
            first = found[0]
            real = [a for a in first if a[0] != "H"]
            check(36 <= len(real) <= 48,
                  "quartz r=5: %d Si and O atoms, expected about 42" % len(real))
            quartz_checks(first, "quartz H caps", 5.0)
            quartz_checks(found[1], "quartz units + OH caps", 5.0)
            atoms = re.findall(r"BUILDER: atoms=(\d+) lattice=(\w+)", text)
            check(atoms[:2] == [(str(len(first)), "no"), ("9", "yes")],
                  "atom count and lattice after the cut and after undo (%s)" % atoms)
        check("FAIL" not in "\n".join(l for l in text.splitlines()
                                       if "ECCE_BUILDER_SCRIPT" in l),
              "no script step failed")

        text = eb.runBuilder(
            s, env, wrappers, "co2",
            ["wait 3000", "selectelem C", "cutcluster 4.5 whole molecules",
             "expect lattice no", "quit"],
            None, structure=os.path.join(fixtures, "co2.cif"))
        found = blocks(text)
        if check(len(found) == 1, "co2: one cluster reported"):
            atoms = found[0]
            nb = neighbours(atoms)
            cs = [i for i, a in enumerate(atoms) if a[0] == "C"]
            os_ = [i for i, a in enumerate(atoms) if a[0] == "O"]
            check(len(cs) > 1 and len(os_) == 2 * len(cs),
                  "co2: %d C and %d O, whole molecules" % (len(cs), len(os_)))
            check(all(len(nb[i]) == 2 for i in cs) and all(len(nb[i]) == 1 for i in os_),
                  "co2: every molecule is intact")
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
