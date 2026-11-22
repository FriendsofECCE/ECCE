#!/usr/bin/env python3
"""Moving-grid XC gradients: net force, translation, and an energy oracle."""
import math
import os
import sys
import tempfile

from gradient_test import STEP, energy, gradient, pushed, run


def main():
    exe = os.path.abspath(sys.argv[1])
    cases = [("LiF", 0, 1, [("Li", 0., 0., 0.), ("F", 0., 0., 3.)]),
             ("water", *pushed("h2o"))]
    bad = 0
    with tempfile.TemporaryDirectory() as work:
        for name, charge, mult, atoms in cases:
            def evaluate(geometry, runtype, grid):
                return run(exe, work, "point", charge, mult, geometry,
                           "b3lyp", "6-31G*", runtype, grid)

            for grid in (None, (150, 974)):
                text = evaluate(atoms, "gradient", grid)
                ana = gradient(text)
                net = [sum(ana[k::3]) for k in range(3)]
                norm = math.sqrt(sum(v * v for v in net))
                ok = math.isfinite(norm) and norm < 1e-6
                bad += not ok
                print(f"{name} B3LYP/6-31G* grid={grid or 'default'} "
                      f"sum(gradient)={net} norm={norm:.3e} Eh/bohr "
                      f"{'PASS' if ok else 'FAIL'}", flush=True)
                if grid is not None:
                    continue

                # Check every component against the energy of a rebuilt grid.
                fd = []
                for a in range(len(atoms)):
                    for c in range(3):
                        energies = []
                        for sign in (1, -1):
                            moved = [list(t) for t in atoms]
                            moved[a][1 + c] += sign * STEP
                            energies.append(energy(evaluate([tuple(t) for t in moved], "energy", grid)))
                        fd.append((energies[0] - energies[1]) / (2 * STEP))
                errors = [abs(x - y) for x, y in zip(ana, fd)]
                ok = all(math.isfinite(v) and v < 1e-5 for v in errors)
                bad += not ok
                print(f"{name} step={STEP:g} bohr max|analytic-FD|={max(errors):.3e} "
                      f"Eh/bohr {'PASS' if ok else 'FAIL'}", flush=True)
                if not ok:
                    print(f"  analytic={ana}\n  FD={fd}", flush=True)

                shift = (0.31, -0.23, 0.17)
                moved = [(a[0], *(a[c + 1] + shift[c] for c in range(3))) for a in atoms]
                shifted = evaluate(moved, "gradient", grid)
                de = abs(energy(shifted) - energy(text))
                dg = max(abs(x - y) for x, y in zip(ana, gradient(shifted)))
                ok = math.isfinite(de) and math.isfinite(dg) and de < 1e-9 and dg < 1e-8
                bad += not ok
                print(f"{name} rigid translation |dE|={de:.3e} Eh "
                      f"max|dgradient|={dg:.3e} Eh/bohr {'PASS' if ok else 'FAIL'}", flush=True)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
