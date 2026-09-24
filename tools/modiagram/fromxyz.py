#!/usr/bin/env python3
"""Turn an XYZ file into a diagram spec, by asking MOPAC for a spectrum.

    modiagram-mopac molecule.xyz spec.mol [charge]

An XYZ file has no orbitals in it.  The standalone MO diagram program
needs energies, occupancies and the orbitals' own symmetry labels, so a
quick semiempirical calculation is run to get them -- which is also how
the test suite gets a real spectrum for a molecule nobody has an ECCE
calculation for.

The reading of MOPAC's output lives in frommopac.py and is shared with
the tests: two implementations of one parser drift, and this codebase
has paid for that more than once.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
for candidate in (os.path.join(HERE, "modiagram"),
                  os.path.join(HERE, "..", "tools", "modiagram")):
    if os.path.isfile(os.path.join(candidate, "frommopac.py")):
        sys.path.insert(0, candidate)
        break

try:
    from frommopac import (run, parse, symmetrise, spec, coefficients,
                           basisFromOutput)
except ImportError as why:
    sys.stderr.write("modiagram-mopac: cannot load the MOPAC reader: %s\n"
                     % why)
    sys.exit(2)


def readXyz(path):
    """Atoms from an XYZ file.  The comment line is ignored."""
    lines = open(path, errors="replace").read().splitlines()
    if not lines:
        raise ValueError("empty file")
    try:
        count = int(lines[0].split()[0])
    except (ValueError, IndexError):
        raise ValueError("first line is not an atom count")

    atoms = []
    for line in lines[2:2 + count]:
        parts = line.split()
        if len(parts) < 4:
            continue
        atoms.append((parts[0].capitalize(), float(parts[1]),
                      float(parts[2]), float(parts[3])))
    if len(atoms) != count:
        raise ValueError("expected %d atoms, read %d" % (count, len(atoms)))
    return atoms


def main():
    if len(sys.argv) < 3:
        sys.stderr.write(__doc__)
        return 2

    source, target = sys.argv[1], sys.argv[2]
    charge = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    try:
        atoms = readXyz(source)
    except (ValueError, OSError) as why:
        sys.stderr.write("modiagram-mopac: %s: %s\n" % (source, why))
        return 1

    home = os.environ.get("ECCE_HOME", "")
    autosym = os.path.join(home, "bin", "autosym") if home else "autosym"
    group, cleaned = symmetrise(atoms, autosym=autosym)

    try:
        out = run("molecule", cleaned, charge=charge)
    except Exception as why:
        sys.stderr.write("modiagram-mopac: MOPAC did not run: %s\n" % why)
        return 1

    #  The group MOPAC reports is in its own frame; the one from
    #  autosym goes with the coordinates that were written, so that is
    #  the one the projection operators must use.
    _, energies, labels = parse(out)
    if not energies:
        sys.stderr.write("modiagram-mopac: MOPAC produced no orbitals\n")
        return 1

    basis = basisFromOutput(out, len(cleaned))
    coefs = coefficients(out, len(cleaned))

    electrons = 0
    for symbol, _, _, _ in cleaned:
        electrons += VALENCE.get(symbol, 0)
    electrons -= charge

    spec(target, group, cleaned, energies, labels, electrons,
         charge=charge, coefs=coefs, basis=basis)
    return 0


#  Valence electrons, which is what a semiempirical code carries.
VALENCE = {
    "H": 1, "He": 2,
    "Li": 1, "Be": 2, "B": 3, "C": 4, "N": 5, "O": 6, "F": 7, "Ne": 8,
    "Na": 1, "Mg": 2, "Al": 3, "Si": 4, "P": 5, "S": 6, "Cl": 7, "Ar": 8,
    "K": 1, "Ca": 2, "Ga": 3, "Ge": 4, "As": 5, "Se": 6, "Br": 7, "Kr": 8,
    "Rb": 1, "Sr": 2, "In": 3, "Sn": 4, "Sb": 5, "Te": 6, "I": 7, "Xe": 8,
}


if __name__ == "__main__":
    sys.exit(main())
