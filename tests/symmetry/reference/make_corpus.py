#!/usr/bin/env python3
"""Writes corpus/ (deterministic; seeded).  Committed output, so a port never
needs to run this.  Stage 1 is pure Python: molecules are built as orbits of
seed atoms under the point group's generators, then rotated, shuffled and
distorted.  Stage 2 needs the Fortran autosym/getfrag (--impl): cleansym,
getfrag and genmol take their input in the standard frame gensym's matrices
are written in, which only autosym produces.

    make_corpus.py [--impl <build dir>]
"""
import argparse
import math
import os
import random
import shutil
import sys

import symref as S

# ---- small vector algebra ---------------------------------------------------

def mm(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]

def mv(m, v):
    return [sum(m[i][k] * v[k] for k in range(3)) for i in range(3)]

def rot(axis, deg):
    n = math.sqrt(sum(c * c for c in axis))
    x, y, z = [c / n for c in axis]
    t = math.radians(deg)
    c, s = math.cos(t), math.sin(t)
    C = 1 - c
    return [[c + x * x * C, x * y * C - z * s, x * z * C + y * s],
            [y * x * C + z * s, c + y * y * C, y * z * C - x * s],
            [z * x * C - y * s, z * y * C + x * s, c + z * z * C]]

def refl(n):
    q = sum(c * c for c in n)
    return [[(1.0 if i == j else 0.0) - 2 * n[i] * n[j] / q for j in range(3)] for i in range(3)]

INV = [[-1.0 if i == j else 0.0 for j in range(3)] for i in range(3)]
Z, X, Y = (0, 0, 1), (1, 0, 0), (0, 1, 0)
PHI = (1 + math.sqrt(5)) / 2

def closure(gens):
    key = lambda m: tuple(round(v, 5) + 0.0 for r in m for v in r)
    I = [[1.0, 0, 0], [0, 1.0, 0], [0, 0, 1.0]]
    seen = {key(I): I}
    todo = [I]
    while todo:
        a = todo.pop()
        for g in gens:
            b = mm(g, a)
            if key(b) not in seen:
                seen[key(b)] = b
                todo.append(b)
                assert len(seen) < 300
    return list(seen.values())

CYC = [[0.0, 0, 1], [1, 0, 0], [0, 1, 0]]   # (x,y,z) -> (z,x,y)
GENS = {
    "C1": [], "Cs": [refl(Z)], "Ci": [INV], "C2": [rot(Z, 180)],
    "C3": [rot(Z, 120)], "C2v": [rot(Z, 180), refl(Y)], "C3v": [rot(Z, 120), refl(Y)],
    "C4v": [rot(Z, 90), refl(Y)], "C2h": [rot(Z, 180), refl(Z)], "C3h": [rot(Z, 120), refl(Z)],
    "D2": [rot(Z, 180), rot(X, 180)], "D2h": [rot(Z, 180), rot(X, 180), INV],
    "D3h": [rot(Z, 120), rot(X, 180), refl(Z)], "D4h": [rot(Z, 90), rot(X, 180), refl(Z)],
    "D5h": [rot(Z, 72), rot(X, 180), refl(Z)], "D6h": [rot(Z, 60), rot(X, 180), refl(Z)],
    "D3d": [rot(Z, 120), rot(Y, 180), INV], "D2d": [mm(refl(Z), rot(Z, 90)), refl(Y)],
    "S4": [mm(refl(Z), rot(Z, 90))],
    "Td": [rot(Z, 180), rot(X, 180), CYC, refl((1, -1, 0))],
    "Oh": [rot(Z, 90), CYC, INV],
    "Ih": [INV, rot(Z, 180), CYC, rot((0, 1, PHI), 72)],
}
ORDER = {"C1": 1, "Cs": 2, "Ci": 2, "C2": 2, "C3": 3, "C2v": 4, "C3v": 6, "C4v": 8,
         "C2h": 4, "C3h": 6, "D2": 4, "D2h": 8, "D3h": 12, "D4h": 16, "D5h": 20,
         "D6h": 24, "D3d": 12, "D2d": 8, "S4": 4, "Td": 24, "Oh": 48, "Ih": 120}
for _g, _gens in GENS.items():
    assert len(closure(_gens)) == ORDER[_g], _g

def orbit(group, seeds):
    ops = closure(GENS[group])
    atoms = []
    for s, v in seeds:
        for m in ops:
            w = mv(m, v)
            if not any(a[0] == s and sum((a[i + 1] - w[i]) ** 2 for i in range(3)) < 1e-6
                       for a in atoms):
                atoms.append((s, w[0], w[1], w[2]))
    return atoms

def ring(n, r, z, phase=0.0):
    return [(r * math.cos(math.radians(phase + 360.0 * k / n)),
             r * math.sin(math.radians(phase + 360.0 * k / n)), z) for k in range(n)]

def ico(k):
    return (0, k, k * PHI)

# name, group, seeds [(sym, xyz)] -- real molecules, textbook geometries (A)
MOLS = [
    ("chfclbr", "C1", "C1", [("C", (0, 0, 0)), ("H", (0, 0, 1.09)), ("F", (1.28, 0.0, -0.45)),
                              ("Cl", (-0.95, 1.55, -0.55)), ("Br", (-1.05, -1.75, -0.65))]),
    ("hocl", "Cs", "Cs", [("O", (0, 0, 0)), ("Cl", (1.69, 0, 0)), ("H", (-0.2, 0.93, 0))]),
    ("ch2clf", "Cs", "Cs", [("C", (0, 0, 0)), ("F", (1.36, 0, 0)), ("Cl", (-0.62, 1.55, 0)),
                             ("H", (-0.38, -0.55, 0.88))]),
    ("c2h2f2cl2-anti", "Ci", "Ci", [("C", (0.77, 0.1, 0.05)), ("F", (1.35, 1.2, -0.45)),
                                    ("Cl", (1.7, -1.2, 0.75)), ("H", (1.0, 0.2, 1.1))]),
    ("h2o2", "C2", "C2", [("O", (0.725, 0, 0)), ("H", (0.80, 0.75, 0.55))]),
    ("boh3-propeller", "C3", "C3", [("B", (0, 0, 0)), ("O", (1.37, 0, 0.15)), ("H", (1.75, 0.8, 0.55))]),
    ("h2o", "C2v", "C2v", [("O", (0, 0, 0.1173)), ("H", (0, 0.7572, -0.4692))]),
    ("ch2f2", "C2v", "C2v", [("C", (0, 0, 0)), ("F", (0, 1.10, 0.80)), ("H", (0.90, 0, -0.62))]),
    ("nh3", "C3v", "C3v", [("N", (0, 0, 0)), ("H", (0.94, 0, -0.38))]),
    ("chf3", "C3v", "C3v", [("C", (0, 0, 0)), ("H", (0, 0, 1.09)), ("F", (1.24, 0, -0.45))]),
    ("xeof4", "C4v", "C4v", [("Xe", (0, 0, 0)), ("O", (0, 0, 1.70)), ("F", (1.95, 0, -0.30))]),
    ("trans-c2h2cl2", "C2h", "C2h", [("C", (0.67, 0, 0)), ("Cl", (1.35, 1.45, 0)), ("H", (1.25, -0.93, 0))]),
    ("boh3-planar", "C3h", "C3h", [("B", (0, 0, 0)), ("O", (1.37, 0, 0)), ("H", (1.80, 0.80, 0))]),
    ("twisted-ethene", "D2", "D2", [("C", (0.67, 0, 0)), ("H", (1.24, 0.93, 0.16))]),
    ("ethene", "D2h", "D2h", [("C", (0.667, 0, 0)), ("H", (1.24, 0.92, 0))]),
    ("bf3", "D3h", "D3h", [("B", (0, 0, 0)), ("F", (1.31, 0, 0))]),
    ("xef4", "D4h", "D4h", [("Xe", (0, 0, 0)), ("F", (1.95, 0, 0))]),
    ("c5h5", "D5h", "D5h", [("C", (1.2, 0, 0)), ("H", (2.2, 0, 0))]),
    ("benzene", "D6h", "D6h", [("C", (1.40, 0, 0)), ("H", (2.48, 0, 0))]),
    ("ethane", "D3d", "D3d", [("C", (0, 0, 0.765)), ("H", (1.03, 0, 1.15))]),
    ("allene", "D2d", "D2d", [("C", (0, 0, 0)), ("C", (0, 0, 1.31)), ("H", (0.94, 0, 1.88))]),
    ("s4-generic", "S4", "S4", [("C", (0, 0, 0)), ("N", (1.3, 0.4, 0.5)), ("O", (2.4, 0.9, 0.2))]),
    ("ch4", "Td", "Td", [("C", (0, 0, 0)), ("H", (0.629, 0.629, 0.629))]),
    ("ccl4", "Td", "Td", [("C", (0, 0, 0)), ("Cl", (1.02, 1.02, 1.02))]),
    ("p4", "Td", "Td", [("P", (0.78, 0.78, 0.78))]),
    ("sf6", "Oh", "Oh", [("S", (0, 0, 0)), ("F", (1.56, 0, 0))]),
    ("b12h12", "Ih", "Ih", [("B", ico(0.89)), ("H", ico(1.52))]),
]
# linear molecules, along z (autosym has no infinite groups, see the entry)
LINEAR = [
    ("hcn", "Cinfv", [("H", (0, 0, -1.065)), ("C", (0, 0, 0)), ("N", (0, 0, 1.156))]),
    ("co", "Cinfv", [("C", (0, 0, 0)), ("O", (0, 0, 1.128))]),
    ("hcl", "Cinfv", [("H", (0, 0, 0)), ("Cl", (0, 0, 1.275))]),
    ("n2", "Dinfh", [("N", (0, 0, -0.55)), ("N", (0, 0, 0.55))]),
    ("co2", "Dinfh", [("O", (0, 0, -1.16)), ("C", (0, 0, 0)), ("O", (0, 0, 1.16))]),
    ("c2h2", "Dinfh", [("H", (0, 0, -1.66)), ("C", (0, 0, -0.6)), ("C", (0, 0, 0.6)), ("H", (0, 0, 1.66))]),
]
# the molecule on its own, in a second Cartesian frame: ensures the group and
# frame found do not depend on how the input was oriented
DISTORT = [0.003, 0.008, 0.02, 0.06]

def rand_rot(rng):
    ax = [rng.uniform(-1, 1) for _ in range(3)]
    return rot(ax, rng.uniform(0, 360))

def reorient(atoms, rng):
    m = rand_rot(rng)
    t = [rng.uniform(-3, 3) for _ in range(3)]
    out = []
    for s, x, y, z in atoms:
        v = mv(m, (x, y, z))
        out.append((s, v[0] + t[0], v[1] + t[1], v[2] + t[2]))
    rng.shuffle(out)
    return out

def distort(atoms, amp, rng):
    return [(s, x + rng.uniform(-amp, amp), y + rng.uniform(-amp, amp),
             z + rng.uniform(-amp, amp)) for s, x, y, z in atoms]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--impl", default=S.default_impl())
    a = ap.parse_args()
    if os.path.isdir(S.CORPUS):
        shutil.rmtree(S.CORPUS)
    seed = [0]
    def rng_for(name):
        return random.Random("symref:" + name)
    P = lambda prog, name: os.path.join(S.CORPUS, prog, name + ".case")

    mols = [(n, g, orbit(gk, sd)) for n, g, gk, sd in MOLS]
    mols += [(n, g, [(s, v[0], v[1], v[2]) for s, v in at]) for n, g, at in LINEAR]

    # ---- autosym: exact, reoriented+shuffled, distorted, other thresholds
    for n, g, at in mols:
        write = lambda tag, atoms, thr=0.01: S.write_case(
            P("autosym", "%s.%s" % (n, tag)), "autosym", atoms, threshold=thr,
            note="%s, nominal group %s, %s" % (n, g, tag))
        write("exact", at)
        write("rot", reorient(at, rng_for(n + "rot")))
        for amp in DISTORT:
            write("d%g" % amp, reorient(distort(at, amp, rng_for("%s%g" % (n, amp))), rng_for(n + "o")))
    for n in ("h2o", "ch4", "benzene", "nh3", "ethene", "sf6"):
        at = dict((m[0], m[2]) for m in mols)[n]
        for thr in (0.001, 0.05, 0.1):
            S.write_case(P("autosym", "%s.exact.t%g" % (n, thr)), "autosym", at, threshold=thr,
                         note="%s exact at threshold %g" % (n, thr))
            S.write_case(P("autosym", "%s.d0.02.t%g" % (n, thr)), "autosym",
                         distort(at, 0.02, rng_for("%s0.02" % n)), threshold=thr,
                         note="%s distorted 0.02 at threshold %g" % (n, thr))

    # ---- symops: every group name, plus unknown / lower case
    for gname in ["C1", "CS", "CI", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "D2", "D3", "D4",
                  "D5", "D6", "C2V", "C3V", "C4V", "C5V", "C6V", "C2H", "C3H", "C4H", "C5H",
                  "C6H", "D2H", "D3H", "D4H", "D5H", "D6H", "D8H", "D2D", "D3D", "D4D", "D5D",
                  "D6D", "S4", "S6", "S8", "T", "TH", "TD", "O", "OH", "I", "IH",
                  "XX", "C2v", "CINFV"]:
        S.write_case(P("symops", gname), "symops", [], group=gname)

    # ---- genmollat: space groups, general and special positions
    cells = {"tric": [5, 6, 7, 80, 95, 100], "mono": [5, 6, 7, 90, 100, 90],
             "orth": [5, 6, 7, 90, 90, 90], "tet": [5, 5, 7, 90, 90, 90],
             "hex": [5, 5, 7, 90, 90, 120], "cub": [5, 5, 5, 90, 90, 90]}
    sg = [("tric", "P1 P-1"),
          ("mono", "P2 P2_1 C2 Pm Pc Cm P2/m P2_1/m C2/m P2/c P2_1/c C2/c"),
          ("orth", "P222 P2_12_12_1 C222_1 Pmm2 Pna2_1 Pnma Cmcm Fddd Immm Pbca"),
          ("tet", "P4 P4_1 I4_1/a P4/mmm P4_2/mnm"),
          ("hex", "P3 R-3 P3_121 R-3m P-3m1 P6_3/mmc P6/mmm P6_122"),
          ("cub", "P23 Fm-3m Fd-3m Pm-3m Ia-3d I4_132")]
    for system, names in sg:
        for name in names.split():
            for spell in {name, name.replace("_", "")}:
                tag = spell.replace("/", "s")
                S.write_case(P("genmollat", tag), "genmollat", [("C", 0.12, 0.21, 0.33)],
                             threshold=0.1, group=spell, lattice=cells[system],
                             note="%s; the Builder passes the name as in data/client/config/SpaceGroups" % spell)
    for off in (0.0, 0.04, 0.12):
        S.write_case(P("genmollat", "P2.nearaxis%g" % off), "genmollat",
                     [("C", off, 0.25, 0.0 + off / 2)], threshold=0.1, group="P2",
                     lattice=cells["mono"], note="atom %g from the 2-fold axis, threshold 0.1" % off)
        S.write_case(P("genmollat", "Pmmm.nearplane%g" % off), "genmollat",
                     [("O", off, 0.2, 0.3)], threshold=0.1, group="Pmmm", lattice=cells["orth"],
                     note="atom %g from a mirror plane" % off)
    S.write_case(P("genmollat", "P1.two-atoms"), "genmollat",
                 [("Na", 0.0, 0.0, 0.0), ("Cl", 0.5, 0.5, 0.5)], threshold=0.1, group="Fm-3m",
                 lattice=cells["cub"], note="NaCl motif")

    # ---- stage 2: standard-frame inputs from autosym / getfrag
    for n, g, at in mols:
        c = {"program": "autosym", "threshold": 0.01, "group": "", "lattice": None, "atoms": at}
        rc, out = S.run_case(a.impl, c)
        assert rc == 0, n
        res = S.parse_output("autosym", out)
        group = res["group"]
        framed = [(at[i][0],) + tuple(res["xyz"][i]) for i in range(len(at))]
        for amp, thr in ((0.004, 0.01), (0.03, 0.05)):
            S.write_case(P("cleansym", "%s.d%g" % (n, amp)), "cleansym",
                         [(s, x, y, z) for s, x, y, z in distort(framed, amp, rng_for("cl%s%g" % (n, amp)))],
                         threshold=thr, group=group,
                         note="%s in autosym's frame (%s), distorted %g" % (n, group, amp))
        # the Builder's Generate / Find Irreducible / Calculation Editor path
        S.write_case(P("getfrag", n), "getfrag", framed, threshold=0.01, group=group,
                     note="%s full molecule in autosym's frame, group %s" % (n, group))
        S.write_case(P("genmol", n + ".full"), "genmol", framed, threshold=0.01, group=group,
                     note="full molecule is returned unchanged (docs/claude/codereg/genmol-is-handed-the-whole-molecule.md)")
        gc = {"program": "getfrag", "threshold": 0.01, "group": group, "lattice": None, "atoms": framed}
        rc, out = S.run_case(a.impl, gc)
        if rc == 0:
            idx = S.parse_output("getfrag", out)["idx"]
            irr = [framed[i - 1] for i in idx]
            S.write_case(P("genmol", n + ".irr"), "genmol", irr, threshold=0.01, group=group,
                         note="getfrag's irreducible atoms of %s, group %s" % (n, group))
    # a hand-written irreducible set and a name genmol does not know
    S.write_case(P("genmol", "h2o.hand"), "genmol", [("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692)],
                 threshold=0.01, group="C2V", note="hand-written irreducible water")
    S.write_case(P("genmol", "unknown-group"), "genmol", [("O", 0, 0, 0.1)], threshold=0.01,
                 group="C*V", note="Gaussian's linear name: exit status 3")
    S.write_case(P("cleansym", "unknown-group"), "cleansym", [("O", 0, 0, 0.1)], threshold=0.01,
                 group="D*H", note="exit status 3")
    n = sum(len(f) for _, _, f in os.walk(S.CORPUS))
    print("corpus written")

if __name__ == "__main__":
    main()
