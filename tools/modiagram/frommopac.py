#!/usr/bin/env python3
"""Run MOPAC on a molecule and write a diagram spec from its output.

MOPAC prints exactly what a correlation diagram needs and ECCE's parser
keeps most of it: the point group, the orbital energies, and the
orbitals' own Mulliken symbols, already numbered ("1 a1  1 t2 ...").
This is the fastest way to get a real spectrum for a molecule that has
no ECCE calculation on disk.
"""
import os, re, subprocess, sys, tempfile

def run(name, atoms, charge=0, keywords="PM7 1SCF VECTORS ALLVEC"):
    work = tempfile.mkdtemp(prefix="modiag-")
    mop = os.path.join(work, name + ".mop")
    head = keywords + (" CHARGE=%d" % charge if charge else "")
    lines = [head, name, ""]
    for symbol, x, y, z in atoms:
        lines.append(" %-3s %14.8f 0 %14.8f 0 %14.8f 0" % (symbol, x, y, z))
    open(mop, "w").write("\n".join(lines) + "\n")

    subprocess.run(["mopac", mop], cwd=work, capture_output=True, timeout=300)
    out = open(os.path.join(work, name + ".out"), errors="replace").read()
    return out


VALENCE_AOS = {"H": 1, "He": 1}     # s only; everything else s + p


def basisCount(symbol):
    """How many valence AOs MOPAC gives an atom: 1 for H, otherwise 4."""
    return VALENCE_AOS.get(symbol, 4)


def coefficients(out, natoms):
    """The MO coefficients, one row per orbital, from the VECTORS block.

    MOPAC prints them in blocks of eight columns: a row of eigenvalues,
    a row of symmetry labels, then one row per basis function.  The
    rows arrive transposed from what a coefficient table wants, so they
    are collected per block and stitched together.
    """
    body = out.split("EIGENVECTORS")[-1]
    blocks, current = [], []
    for line in body.splitlines():
        t = line.split()
        #  A coefficient row starts with the orbital type and the atom
        #  symbol: "S    C    1" or "PX   O    2".
        #  "S   O    1    0.8516  ..." -- orbital type, element, atom
        #  number, then the coefficients.  MOPAC writes the type in
        #  mixed case ("Px", not "PX").
        if (len(t) > 3
                and re.fullmatch(r"[SPDFspdf][a-zA-Z0-9]*", t[0])
                and re.fullmatch(r"[A-Z][a-z]?", t[1])
                and re.fullmatch(r"\d+", t[2])
                and all(re.fullmatch(r"-?\d+\.\d+", x) for x in t[3:])):
            current.append([float(x) for x in t[3:]])
        elif current:
            blocks.append(current)
            current = []
    if current:
        blocks.append(current)

    rows = []
    for block in blocks:
        width = max(len(r) for r in block)
        for col in range(width):
            rows.append([r[col] if col < len(r) else 0.0 for r in block])
    return rows


def parse(out):
    """(group, [(energy_eV, label)]) from a MOPAC output."""
    group = ""
    m = re.search(r"FOR POINT-GROUP\s+(\S+)", out)
    if m:
        group = m.group(1)

    #  The eigenvector block: rows of bare numbers are eigenvalues, and
    #  the row of "<n> <symbol>" just above them carries the labels.
    body = out.split("EIGENVECTORS")[-1]
    energies, labels = [], []
    for line in body.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if re.fullmatch(r"(-?\d+\.\d+\s*)+", stripped):
            energies += [float(x) for x in stripped.split()]
        elif re.fullmatch(r"(\d+\s+[a-zA-Z][a-zA-Z0-9'\"]*\s*)+", stripped):
            labels += re.findall(r"\d+\s+([a-zA-Z][a-zA-Z0-9'\"]*)", stripped)
    return group, energies, labels


Z = {"H": 1, "C": 6, "N": 7, "O": 8, "F": 9, "S": 16, "Cl": 17}


def symmetrise(atoms, threshold=0.01, autosym="/opt/ecce/bin/autosym"):
    """The point group AND the cleaned, reoriented coordinates.

    Both, because gensym's operation matrices are written in each
    group's standard frame: the group name alone is no use if the atoms
    are not in that frame, which is what "the structure is not in the
    symmetry frame of C3V" means.  ECCE's panel runs the same step for
    the same reason.
    """
    lines = ["%d" % len(atoms), "%g" % threshold]
    for symbol, x, y, z in atoms:
        lines.append("%-16s" % symbol)
        lines.append("%d %G %G %G" % (Z[symbol], x, y, z))
    run = subprocess.run([autosym], input="\n".join(lines) + "\n",
                         capture_output=True, text=True)
    out = run.stdout.split("\n")
    if not out or not out[0].strip():
        return "C1", atoms
    group = out[0].strip()
    cleaned = []
    for i, (symbol, _, _, _) in enumerate(atoms):
        parts = out[1 + i].split()
        cleaned.append((symbol, float(parts[0]), float(parts[1]),
                        float(parts[2])))
    return group, cleaned


def spec(path, group, atoms, energies, labels, electrons, charge=0,
         coefs=None):
    HARTREE = 27.211386245988
    lines = ["group %s" % group]
    if charge:
        lines.append("charge %d" % charge)
    for symbol, x, y, z in atoms:
        lines.append("atom %s %.6f %.6f %.6f" % (symbol, x, y, z))
    lines.append("basis " + " ".join(str(basisCount(s)) for s, _, _, _ in atoms))
    for row in (coefs or []):
        lines.append("coef " + " ".join("%.6f" % c for c in row))
    left = electrons
    for i, e in enumerate(energies):
        occ = 2.0 if left >= 2 else (1.0 if left == 1 else 0.0)
        left -= int(occ)
        lines.append("orbital %.8f %.1f %s"
                     % (e / HARTREE, occ, labels[i] if i < len(labels) else ""))
    open(path, "w").write("\n".join(lines) + "\n")
    return len(energies), len(labels)
