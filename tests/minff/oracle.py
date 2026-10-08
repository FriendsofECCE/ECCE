#!/usr/bin/env python3
"""Reference types and charges straight from atomipy's own MINFF pipeline.

Does not use scripts/minff_topology.py or atomipy's itp writer: the per-atom
'type' and 'charge' fields are printed as JSON.  Run with a python that has
atomipy.

    oracle.py CELL NA NB NC [FROM TO COUNT SEED MINDIST]
"""
import contextlib
import json
import sys

import atomipy as ap

cell, na, nb, nc = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
with contextlib.redirect_stdout(sys.stderr):
    atoms, box = ap.import_auto(cell)
    if (na, nb, nc) != (1, 1, 1):
        atoms, box, _ = ap.replicate_system(atoms, box, [na, nb, nc])
    if len(sys.argv) > 5:
        import numpy as np
        np.random.seed(int(sys.argv[8]))
        atoms, box, _ = ap.substitute(atoms, box, int(sys.argv[7]), sys.argv[5],
                                      sys.argv[6], float(sys.argv[9]))
    atoms = ap.minff(atoms, box)
json.dump({"types": [a["type"] for a in atoms],
           "charges": [a["charge"] for a in atoms]}, sys.stdout)
