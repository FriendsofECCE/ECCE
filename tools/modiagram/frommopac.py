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


#  MOPAC's own names for the valence functions, and the angular
#  momentum each one is.  The d labels carry NO leading D -- they are
#  written "x2  Cl  2", "xz", "z2", "yz", "xy" -- so a rule that looked
#  for an S, P or D at the front of the label dropped all five of them
#  on every chlorine, and CCl4 came out with 20 basis functions against
#  its 40 orbitals.  The basis-versus-labels check is what caught it.
ORBITAL_L = {
    "s": 0,
    "px": 1, "py": 1, "pz": 1,
    "x2": 2, "xz": 2, "z2": 2, "yz": 2, "xy": 2,
    "x2-y2": 2,
}


def basisFromOutput(out, natoms):
    """(functions per atom, angular momentum per function), read from
    MOPAC's own coefficient row labels.

    NOT assumed.  "one function for hydrogen and four for everything
    else" is wrong for anything PM7 gives d orbitals to: chlorine has
    nine, so CCl4 has forty functions and not twenty.  That assumption
    would have produced a confidently wrong diagram, and the
    basis-versus-labels check is what caught it -- the basis spanned 20
    functions while the code reported 40 orbitals.

    Only the FIRST column block is counted.  MOPAC prints the whole
    row set again for every eight orbitals, so counting all of them
    multiplied every atom's basis by the number of blocks.

    The rows are labelled "S  Cl  2", "Px  Cl  2", "Dxy  Cl  2", so the
    output says exactly which functions each atom has and in what
    order.
    """
    perAtom = [0]*natoms
    shells = [[] for _ in range(natoms)]

    lines = finalBlock(out).splitlines()
    started = False

    for line in lines:
        t = line.split()

        #  The second "Root No." ends the first block.
        if len(t) > 2 and t[0] == "Root" and t[1] == "No.":
            if started:
                break
            continue

        if not (len(t) > 3
                and t[0].lower() in ORBITAL_L
                and re.fullmatch(r"[A-Z][a-z]?", t[1])
                and re.fullmatch(r"\d+", t[2])):
            continue

        atom = int(t[2]) - 1
        if not 0 <= atom < natoms:
            continue
        started = True
        shells[atom].append(ORBITAL_L[t[0].lower()])
        perAtom[atom] += 1

    flat = []
    for a in range(natoms):
        flat += shells[a]
    return perAtom, flat


def finalBlock(out):
    """The LAST printing of the eigenvectors, and only that one.

    MOPAC prints them more than once -- CCl4's output has three
    EIGENVECTORS sections and five "Root No." blocks -- and reading
    across printings gives twice as many orbitals as the molecule has.
    That is what the basis-versus-labels check caught: the basis spanned
    20 functions and the labels claimed 40.

    The final printing starts at the last "Root No." line whose first
    root is 1.
    """
    lines = out.splitlines()
    start = 0
    for i, line in enumerate(lines):
        t = line.split()
        if len(t) > 2 and t[0] == "Root" and t[1] == "No." and t[2] == "1":
            start = i
    return "\n".join(lines[start:])


def coefficients(out, natoms):
    """The MO coefficients, one row per orbital.

    Collected per "Root No." block, like parse(), rather than by
    watching for runs of matching lines: a block's rows are broken up
    by blank lines and page headers, and treating each run as a block
    split one block into several and stitched them back in the wrong
    order.

    MOPAC prints them transposed from what a coefficient table wants --
    a row per basis function, a column per orbital -- so each block is
    turned on its side and the blocks are laid end to end.
    """
    lines = finalBlock(out).splitlines()
    rows = []
    block = []

    def flush(block):
        if not block:
            return []
        width = max(len(r) for r in block)
        return [[r[col] if col < len(r) else 0.0 for r in block]
                for col in range(width)]

    for line in lines:
        t = line.split()
        if len(t) > 2 and t[0] == "Root" and t[1] == "No.":
            rows += flush(block)
            block = []
            continue
        if (len(t) > 3
                and t[0].lower() in ORBITAL_L
                and re.fullmatch(r"[A-Z][a-z]?", t[1])
                and re.fullmatch(r"\d+", t[2])
                and all(re.fullmatch(r"-?\d+\.\d+", x) for x in t[3:])):
            block.append([float(x) for x in t[3:]])

    rows += flush(block)
    return rows


def parse(out):
    """(group, energies in eV, labels) from a MOPAC output.

    Parsed STRUCTURALLY, from the "Root No." headers, rather than by
    recognising rows that look like numbers.  MOPAC prints several
    numeric tables after the eigenvectors, and a "row of floats" rule
    swept them up: CCl4 came out with 40 orbitals where it has 20.
    That was caught by the basis-versus-labels check, not by reading
    the output.

    Each block is:  Root No. ...  /  labels  /  eigenvalues  /  rows.
    """
    group = ""
    m = re.search(r"FOR POINT-GROUP\s+(\S+)", out)
    if m:
        group = m.group(1)

    lines = finalBlock(out).splitlines()
    energies, labels = [], []

    i = 0
    while i < len(lines):
        t = lines[i].split()
        if not (len(t) > 2 and t[0] == "Root" and t[1] == "No."):
            i += 1
            continue

        #  The two rows that follow, skipping blanks: symmetry labels,
        #  then the eigenvalues themselves.
        rows = []
        j = i + 1
        while j < len(lines) and len(rows) < 2:
            if lines[j].strip():
                rows.append(lines[j])
            j += 1
        if len(rows) < 2:
            break

        labels += re.findall(r"\d+\s+([a-zA-Z][a-zA-Z0-9'\"]*)", rows[0])
        energies += [float(x) for x in re.findall(r"-?\d+\.\d+", rows[1])]
        i = j

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
    run_ = subprocess.run([autosym], input="\n".join(lines) + "\n",
                          capture_output=True, text=True)
    out = run_.stdout.split("\n")
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
         coefs=None, basis=None):
    HARTREE = 27.211386245988
    lines = ["group %s" % group]
    if charge:
        lines.append("charge %d" % charge)
    for symbol, x, y, z in atoms:
        lines.append("atom %s %.6f %.6f %.6f" % (symbol, x, y, z))
    if basis is not None:
        perAtom, shells = basis
        lines.append("basis " + " ".join(str(n) for n in perAtom))
        lines.append("shells " + " ".join(str(x) for x in shells))
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
