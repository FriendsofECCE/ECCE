#!/usr/bin/env python3
"""The Periodic Builder's Layer builder in a real Builder, headless.

    tests/apps/pbc_layers_test.py            own Xvfb and services

Reuses pbc_edit_test's harness (a Builder on a water calculation, a scene
script, structure dumps).  Each case draws a layer, presses Build Layers
and checks the dump: atom counts, the new cell, bonds inside the layer,
water as H2O molecules.  The geometry itself is checked against published
structures in tests/layerstack (layer_stack_test).
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import pbc_edit_test as pe                                    # noqa: E402

#  Methane in the 5 A cube Create Lattice makes is the layer.
LAYER = ["cmd clear", "cmd add", "cmd addh", "pbcopen", "pbcpress create",
         "pbcpress generate"]

CASES = (
    #  name, script, expected counts, "heavy" (unused)
    ("stack", LAYER + ["pblayer layers 3", "pblayer spacing 4",
                       "pblayer shifta 0.5", "pbcpress layers"],
     {"C": 3, "H": 12}, "C"),
    ("stack-undo", LAYER + ["pblayer layers 3", "pblayer spacing 4",
                            "pbcpress layers", "fragdump built", "undo"],
     {"C": 1, "H": 4}, "C"),
    ("water-ions", LAYER + ["pblayer layers 2", "pblayer spacing 9",
                            "pblayer water 4", "pblayer ion1 Na",
                            "pblayer ion1n 2", "pblayer seed 5",
                            "pbcpress layers"],
     {"C": 2, "H": 8 + 8, "O": 8, "Na": 4}, "C"),
)


def problems(work, case):
    name, script, expected, _heavy = case
    found = []
    final = pe.readDump(os.path.join(work, "final.txt"))
    have = {}
    for sym, _xyz, _nb, _f in final["atoms"]:
        have[sym] = have.get(sym, 0) + 1
    for sym, n in sorted(expected.items()):
        if have.get(sym, 0) != n:
            found.append("final: %d %s, expected %d"
                         % (have.get(sym, 0), sym, n))
    if final["counts"].get("lattice") != 1:
        found.append("final: no lattice")
        return found
    vec = final["vectors"]
    cz = vec[2][2]
    if name == "stack" and abs(cz - 12.0) > 1e-6:
        found.append("final: c height %.4f, expected 3 x 4 = 12" % cz)
    if name == "stack-undo" and abs(cz - 5.0) > 1e-6:
        found.append("final: c height %.4f after undo, expected 5" % cz)
    if name == "stack-undo":
        built = pe.readDump(os.path.join(work, "built.txt"))
        if len(built["atoms"]) != 15:
            found.append("built: %d atoms before undo" % len(built["atoms"]))
    if name == "water-ions":
        atoms = final["atoms"]
        for i, (sym, xyz, nb, _f) in enumerate(atoms):
            if sym != "O":
                continue
            hs = [a for a in atoms[i:i + 3] if a[0] == "H"]
            if len(hs) != 2 or nb != 2:
                found.append("water at %s: %d H follow, %d bonds"
                             % (xyz, len(hs), nb))
                continue
            d = [pe.imageDistance(xyz, h[1], vec) for h in hs]
            if any(abs(x - 0.9572) > 0.01 for x in d):
                found.append("water at %s: O-H %s" % (xyz, d))
    return found


def main():
    pe.CASES = CASES
    pe.problems = problems
    pe.FIXTURE = "pbc-layers-water"
    return pe.main()


if __name__ == "__main__":
    sys.exit(main())
