#!/usr/bin/env python3
"""Reference EHT energies from YAeHMOP's `bind` (the oracle for FragmentOrbitals).

    make_yaehmop_ref.py BIND OUTDIR

Writes each test molecule as OUTDIR/NAME.xyz and, from `bind`, the orbital
energies in OUTDIR/NAME.eht ("index energy_eV", ascending).  bind uses the
same parameter set (eht_parms.dat) and the weighted Wolfsberg-Helmholz
formula by default, but exact Slater overlaps, so the numbers are an
independent check of our STO-nG engine.  The checked-in copies live in
tests/fragorb/reference/.
"""
import math, os, re, subprocess, sys

def octa(centre, ligand_atoms_fn):
    """Atoms of M + six ligands along +-x,+-y,+-z."""
    atoms = [(centre, 0.0, 0.0, 0.0)]
    for ax in range(3):
        for sgn in (1, -1):
            R = [[0,0,1],[1,0,0],[0,1,0]]  # local z -> axis
            for el, x, y, z in ligand_atoms_fn():
                v = [x, y, z]
                # rotate local frame so local z points along sgn*axis
                g = [0.0, 0.0, 0.0]
                g[ax] = sgn*v[2]
                g[(ax+1) % 3] = v[0]
                g[(ax+2) % 3] = v[1]
                atoms.append((el, *g))
    return atoms

def co(d=1.91): return [("C", 0, 0, d), ("O", 0, 0, d+1.14)]
def cl():     return [("Cl", 0, 0, 2.36)]
def f():      return [("F", 0, 0, 1.85)]
def nh3():
    d = 1.96; r = 1.01; th = math.radians(70.0)
    out = [("N", 0, 0, d)]
    for k in range(3):
        ph = math.radians(120*k)
        out.append(("H", r*math.sin(th)*math.cos(ph), r*math.sin(th)*math.sin(ph),
                    d + r*math.cos(th)))
    return out

MOLS = {
  "co":     ([("C",0,0,0),("O",0,0,1.128)], 10, 0),
  "crco6":  (octa("Cr", co), 66, 0),
  # metals whose double-zeta d coefficients are not normalised in the source file
  "moco6":  (octa("Mo", lambda: co(2.06)), 66, 0),
  "wco6":   (octa("W", lambda: co(2.06)), 66, 0),
  "nico6":  (octa("Ni", lambda: co(1.90)), 70, 0),
  "tif6":   (octa("Ti", f), 48, -2),
  "conh36": (octa("Co", nh3), 54, 3),
}

def main():
    bind, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    for name, (atoms, electrons, charge) in MOLS.items():
        with open(os.path.join(out, name + ".xyz"), "w") as fh:
            fh.write("%d\n%s charge %d\n" % (len(atoms), name, charge))
            for a in atoms: fh.write("%s %.6f %.6f %.6f\n" % a)
        inp = os.path.join(out, name + ".bind")
        with open(inp, "w") as fh:
            fh.write("[%s]\nmolecular\ngeometry\n%d\n" % (name, len(atoms)))
            for i, a in enumerate(atoms):
                fh.write("%d %s %.6f %.6f %.6f\n" % ((i+1,)+a))
            fh.write("electrons\n%d\nprint\norbital energies\nend_print\n" % electrons)
        subprocess.run([bind, inp], check=True, stdout=subprocess.DEVNULL)
        rows = []
        for ln in open(inp + ".out"):
            m = re.match(r"\s*(\d+):--->\s+(-?[\d.]+)\s+\[", ln)
            if m: rows.append((int(m.group(1)), float(m.group(2))))
        with open(os.path.join(out, name + ".eht"), "w") as fh:
            for i, e in rows: fh.write("%d %.4f\n" % (i, e))
        print(name, len(rows), "orbitals")

main()
