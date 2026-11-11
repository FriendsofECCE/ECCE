#!/usr/bin/env python3
"""Sample structures for ecce-xrd (#971).

    make_samples.py helix OUT.mvm [--residues 20]      polyalanine alpha-helix
    make_samples.py xyz2mvm IN.xyz OUT.mvm TITLE        bonds by covalent radius

The helix is built from internal coordinates (phi -57, psi -47, omega 180, N-CA-C 109.5,
Engh-Huber bond lengths and other angles; N-CA-C 111.2 would give a 5.64 A pitch), so its rise and turn are a
result of the torsions and can be checked independently.
"""
import argparse
import math
import sys

import numpy as np

RADII = {"H": 0.31, "C": 0.76, "N": 0.71, "O": 0.66, "P": 1.07, "S": 1.05}


def place(a, b, c, bond, angle, torsion):
    """Atom d bonded to c: |cd| = bond, angle b-c-d, torsion a-b-c-d (degrees)."""
    bc = c - b
    bc /= np.linalg.norm(bc)
    n = np.cross(b - a, bc)
    n /= np.linalg.norm(n)
    m = np.cross(n, bc)
    t, w = math.radians(torsion), math.radians(angle)
    return c + bond * (-math.cos(w) * bc + math.sin(w) * (math.cos(t) * m + math.sin(t) * n))


def unit(v):
    return v / np.linalg.norm(v)


def polyalanine(nres, phi=-57.0, psi=-47.0):
    atoms = []
    n = np.array([0.0, 0.0, 0.0])
    ca = np.array([1.458, 0.0, 0.0])
    c = ca + 1.525 * np.array([-math.cos(math.radians(109.5)), math.sin(math.radians(109.5)), 0.0])
    prev_c = None
    for i in range(nres):
        o = place(n, ca, c, 1.231, 120.8, psi + 180.0)
        cb = ca + (-0.58273431 * np.cross(ca - n, c - ca) + 0.56802827 * (ca - n)
                   - 0.54067466 * (c - ca)) * (1.53 / 1.52)
        ha = ca + 1.09 * unit(-(unit(n - ca) + unit(c - ca) + unit(cb - ca)))
        if prev_c is None:
            hn = [place(c, ca, n, 1.01, 109.5, 60.0), place(c, ca, n, 1.01, 109.5, 180.0)]
        else:
            hn = [n + 1.01 * unit(-(unit(prev_c - n) + unit(ca - n)))]
        hb = [place(n, ca, cb, 1.09, 109.5, t) for t in (60.0, 180.0, 300.0)]
        atoms += [("N", n)] + [("H", h) for h in hn] + [("C", ca), ("H", ha), ("C", cb)]
        atoms += [("H", h) for h in hb] + [("C", c), ("O", o)]
        n2 = place(n, ca, c, 1.329, 116.2, psi)
        ca2 = place(ca, c, n2, 1.458, 121.7, 180.0)
        c2 = place(c, n2, ca2, 1.525, 109.5, phi)
        prev_c, n, ca, c = c, n2, ca2, c2
    return atoms


def write_xyz(path, atoms, title):
    with open(path, "w") as f:
        f.write("%d\n%s\n" % (len(atoms), title))
        for s, p in atoms:
            f.write("%-2s %12.6f %12.6f %12.6f\n" % (s, *p))


def write_mvm(path, atoms, title, comment):
    xyz = np.array([p for _, p in atoms])
    bonds = []
    for i in range(len(atoms)):
        d = np.linalg.norm(xyz[i + 1:] - xyz[i], axis=1)
        for k in np.nonzero(d < 0.0001 + 0.45 + np.array(
                [RADII[atoms[i][0]] + RADII[atoms[i + 1 + j][0]] for j in range(len(d))]))[0]:
            if d[k] > 0.4:
                bonds.append((i + 1, i + k + 2))
    with open(path, "w") as f:
        f.write("# %s\n" % comment)
        f.write("title:          %s\ntype:           molecule\ncell_units:     angstroms\n"
                "num_atoms:      %d\natom_info:      symbol cart\natom_list:\n" % (title, len(atoms)))
        for s, p in atoms:
            f.write("  %-2s %10.4f %10.4f %10.4f\n" % (s, *p))
        f.write("num_bonds:      %d\nbond_info:     order\nbond_list:\n" % len(bonds))
        for a, b in bonds:
            f.write(" %d  %d  1.00000\n" % (a, b))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    h = sub.add_parser("helix")
    h.add_argument("out")
    h.add_argument("--residues", type=int, default=20)
    x = sub.add_parser("xyz2mvm")
    x.add_argument("inp")
    x.add_argument("out")
    x.add_argument("title")
    a = ap.parse_args()
    if a.cmd == "helix":
        write_mvm(a.out, polyalanine(a.residues), "Alpha-helix, polyalanine %d" % a.residues,
                  "Ideal alpha-helix (phi -57, psi -47), built by tools/fibre/make_samples.py")
    else:
        lines = open(a.inp).read().splitlines()
        atoms = [(l.split()[0], np.array([float(v) for v in l.split()[1:4]]))
                 for l in lines[2:2 + int(lines[0])]]
        write_mvm(a.out, atoms, a.title,
                  "Generated with the Builder's DNA generator, converted by tools/fibre/make_samples.py")
    return 0


if __name__ == "__main__":
    sys.exit(main())
