#!/usr/bin/env python3
"""Check ECCE's PDBQT preparation against Meeko.

Meeko (with RDKit) is a test-only oracle: it is never shipped and never
imported by ECCE itself.  The test skips when it is not installed.
Usage: meeko_compare.py <path to ecce-pdbqt> [workdir]
"""
import os
import subprocess
import sys
import tempfile

LIGANDS = {
    "aspirin":      "CC(=O)Oc1ccccc1C(=O)O",
    "ibuprofen":    "CC(C)Cc1ccc(cc1)C(C)C(=O)O",
    "caffeine":     "Cn1cnc2c1c(=O)n(C)c(=O)n2C",
    "paracetamol":  "CC(=O)Nc1ccc(O)cc1",
    "naproxen":     "COc1ccc2cc(ccc2c1)C(C)C(=O)O",
    "diazepam":     "CN1c2ccc(Cl)cc2C(=NCC1=O)c1ccccc1",
    "warfarin":     "CC(=O)CC(c1ccccc1)c1c(O)c2ccccc2oc1=O",
    "indomethacin": "COc1ccc2c(c1)c(CC(=O)O)c(C)n2C(=O)c1ccc(Cl)cc1",
    "metformin":    "CN(C)C(=N)NC(=N)N",
    "sildenafil":   "CCCc1nn(C)c2c1nc([nH]c2=O)-c1cc(ccc1OCC)S(=O)(=O)N1CCN(C)CC1",
    "imatinib":     "Cc1ccc(cc1Nc1nccc(n1)-c1cccnc1)NC(=O)c1ccc(CN2CCN(C)CC2)cc1",
}


def ecce_info(exe, sdf):
    out = subprocess.run([exe, "--info", sdf], capture_output=True, text=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr)
    types, rot = {}, set()
    for line in out.stdout.splitlines():
        f = line.split()
        if f[0] == "atom":
            types[int(f[1]) - 1] = f[3]
        elif f[0] == "rotatable":
            rot.add(tuple(sorted((int(f[1]) - 1, int(f[2]) - 1))))
    return types, rot


def main():
    exe = sys.argv[1]
    try:
        from rdkit import Chem
        from rdkit.Chem import AllChem
        from meeko import MoleculePreparation
    except ImportError:
        print("meeko/rdkit not installed; skipping")
        return 77

    work = sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp()
    os.makedirs(work, exist_ok=True)
    bad = 0
    for name, smi in sorted(LIGANDS.items()):
        mol = Chem.AddHs(Chem.MolFromSmiles(smi))
        AllChem.EmbedMolecule(mol, randomSeed=20261111)
        AllChem.MMFFOptimizeMolecule(mol)
        sdf = os.path.join(work, name + ".sdf")
        w = Chem.SDWriter(sdf); w.write(mol); w.close()

        setup = MoleculePreparation().prepare(mol)[0]
        # Meeko may open a macrocycle with pseudo-atoms G0/CG0 so the ring
        # itself can flex.  ECCE keeps rings rigid, as AutoDock always has,
        # so normalise those types away before comparing.
        ref_types = {}
        for a in setup.atoms:
            if a.is_dummy or a.atom_type.startswith("G"):
                continue
            t = a.atom_type
            if t.startswith("CG"):
                t = "C"
            elif t.startswith("A") and len(t) > 1:
                t = "A"
            ref_types[a.index] = t
        ref_rot = set()
        for key, bond in setup.bond_info.items():
            if bond.rotatable:
                ref_rot.add(tuple(sorted(key)))

        types, rot = ecce_info(exe, sdf)
        tdiff = [(i, types.get(i), ref_types.get(i))
                 for i in sorted(ref_types) if types.get(i) != ref_types[i]]
        # Meeko flags ring and terminal bonds rotatable and drops them when it
        # builds the tree; compare only the bonds that become real torsions.
        ref_tors = {b for b in ref_rot if is_torsion(setup, b)}
        if tdiff or rot != ref_tors:
            bad += 1
            print("FAIL %s: %d type differences %s; torsions ecce=%d meeko=%d%s"
                  % (name, len(tdiff), tdiff[:6], len(rot), len(ref_tors),
                     "" if rot == ref_tors else
                     " extra=%s missing=%s" % (sorted(rot - ref_tors),
                                               sorted(ref_tors - rot))))
        else:
            print("ok   %s: %d atoms, %d torsions" % (name, len(types), len(rot)))
    print("%d of %d ligands differ" % (bad, len(LIGANDS)))
    return 1 if bad else 0


def is_torsion(setup, bond):
    """A rotatable bond is a torsion only if cutting it splits the molecule
    and neither side is hydrogens alone."""
    import collections
    adj = collections.defaultdict(set)
    heavy = {a.index for a in setup.atoms
             if not a.is_dummy and a.atom_type != "H"}
    for key in setup.bond_info:
        i, j = key
        if i in heavy and j in heavy:
            adj[i].add(j); adj[j].add(i)
    i, j = bond
    if i not in heavy or j not in heavy:
        return False
    if j not in adj[i]:
        return False
    adj[i].discard(j); adj[j].discard(i)
    seen, stack = {i}, [i]
    while stack:
        c = stack.pop()
        for n in adj[c]:
            if n not in seen:
                seen.add(n); stack.append(n)
    split = j not in seen
    return split and len(seen) > 1 and len(heavy - seen) > 1


if __name__ == "__main__":
    sys.exit(main())
