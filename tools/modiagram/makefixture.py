#!/usr/bin/env python3
"""Reduce a real ECCE calculation to a basis/labels oracle fixture.

    makefixture.py <calculation directory> <output.oracle>

WHAT THIS IS FOR.  The oracle in tests/symmetry compares the irreps the
whole AO basis spans against the labels the code itself reports.  The
MOPAC cases there are generated on the fly, which keeps them honest but
limits them to a semiempirical valence basis: four functions an atom,
s and p, no spherical harmonics.  A real basis set is a different
exercise -- def2-SVP puts fourteen functions on carbon including five
spherical d -- and it is the path ECCE actually uses.

So this takes a calculation ECCE has already stored and keeps the four
things the check needs: the geometry, the point group, the basis
layout, and the code's own orbital labels.  Everything else is
discarded, which is why a fixture is a few hundred bytes rather than a
megabyte of output.

Reads ORCA and NWChem.  Each prints its basis layout differently and
neither prints it in a form meant for this, so the parsing is per code
and is checked against the orbital count before the fixture is written.
"""
import os
import re
import subprocess
import sys

L = {"s": 0, "p": 1, "d": 2, "f": 3, "g": 4}


def geometry(calc):
    block = open(os.path.join(calc, "Parameters", "chemsys.mvm")).read()
    block = block.split("atom_list:")[1].split("attr_list:")[0]
    return [(t[0], float(t[1]), float(t[2]), float(t[3]))
            for t in (l.split() for l in block.strip().splitlines())]


def labels(calc):
    path = os.path.join(calc, "Props", "ORBSYM")
    if not os.path.exists(path):
        return []
    return re.sub(r"<[^>]*>", "", open(path).read()).split()


def fromOrca(text, atoms):
    """ORCA names every function in its MO block: "0C   1s", "4O  1dz2"."""
    lines = text.split("MOLECULAR ORBITALS")[-1].splitlines()
    per, seen = {}, None
    for ln in lines:
        m = re.match(r"\s+(\d+)([A-Z][a-z]?)\s+(\d)([spdfg])", ln)
        if not m:
            continue
        atom = int(m.group(1))
        #  The block repeats in column groups; the second one restarts
        #  at atom 0.
        if seen is not None and atom < seen:
            break
        seen = atom
        per.setdefault(atom, []).append(L[m.group(4)])

    if sorted(per) != list(range(len(atoms))):
        return None, None
    perAtom = [len(per[a]) for a in sorted(per)]
    shellOf = [l for a in sorted(per) for l in per[a]]
    return perAtom, shellOf


def fromNwchem(text, atoms):
    """NWChem prints the basis itself, shells numbered per element."""
    if 'Basis "ao basis"' not in text:
        return None, None
    header = re.search(r'Basis "ao basis".*', text).group(0)
    spherical = "spherical" in header

    block = text.split('Basis "ao basis"')[1].split("Summary of")[0]
    shells, element, seen = {}, None, set()
    for ln in block.splitlines():
        m = re.match(r"\s*([A-Z][a-z]?)\s+\(", ln)
        if m:
            element = m.group(1)
            shells[element] = []
            seen = set()
            continue
        m = re.match(r"\s*(\d+)\s+([SPDFG])\s+[-\d.]+E", ln)
        if m and element:
            key = (int(m.group(1)), m.group(2))
            if key in seen:
                continue
            seen.add(key)
            shells[element].append(L[m.group(2).lower()])

    def width(l):
        return (2*l + 1) if spherical else ((l + 1)*(l + 2))//2

    perAtom, shellOf = [], []
    for symbol, _, _, _ in atoms:
        if symbol not in shells:
            return None, None
        perAtom.append(sum(width(l) for l in shells[symbol]))
        for l in shells[symbol]:
            shellOf += [l]*width(l)
    return perAtom, shellOf


def symmetrise(atoms, threshold=0.01, autosym="/opt/ecce/bin/autosym"):
    Z = {"H": 1, "C": 6, "N": 7, "O": 8, "F": 9, "S": 16, "Cl": 17}
    lines = ["%d" % len(atoms), "%g" % threshold]
    for symbol, x, y, z in atoms:
        lines.append("%-16s" % symbol)
        lines.append("%d %G %G %G" % (Z[symbol], x, y, z))
    proc = subprocess.run([autosym], input="\n".join(lines) + "\n",
                          capture_output=True, text=True)
    out = proc.stdout.split("\n")
    if not out or not out[0].strip():
        return "C1", atoms
    cleaned = []
    for i, (symbol, _, _, _) in enumerate(atoms):
        t = out[1 + i].split()
        cleaned.append((symbol, float(t[0]), float(t[1]), float(t[2])))
    return out[0].strip(), cleaned


def main(calc, dest):
    atoms = geometry(calc)
    reported = labels(calc)
    if not reported:
        return "no ORBSYM: nothing to check the basis against"

    outputs = os.path.join(calc, "Outputs")
    perAtom = shellOf = None
    for entry in sorted(os.listdir(outputs)):
        text = open(os.path.join(outputs, entry), errors="replace").read()
        if "O   R   C   A" in text or "MOLECULAR ORBITALS" in text:
            perAtom, shellOf = fromOrca(text, atoms)
        elif "Northwest Computational Chemistry" in text:
            perAtom, shellOf = fromNwchem(text, atoms)
        if perAtom:
            break

    if not perAtom:
        return "could not read a basis layout from the output"

    #  The check that makes the fixture trustworthy: the basis and the
    #  orbital count are arrived at separately and must agree.
    if sum(perAtom) != len(reported):
        return ("basis has %d functions but the code reports %d orbitals"
                % (sum(perAtom), len(reported)))

    group, clean = symmetrise(atoms)
    lines = [group, str(len(clean))]
    for symbol, x, y, z in clean:
        lines.append("%s %.6f %.6f %.6f" % (symbol, x, y, z))
    lines.append(" ".join(str(n) for n in perAtom))
    lines.append(" ".join(str(n) for n in shellOf))
    lines.append(" ".join(reported))
    open(dest, "w").write("\n".join(lines) + "\n")
    return None


if __name__ == "__main__":
    why = main(sys.argv[1], sys.argv[2])
    if why:
        print(why)
        sys.exit(1)
    print("wrote", sys.argv[2])
