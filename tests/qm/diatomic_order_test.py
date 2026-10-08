#!/usr/bin/env python3
"""Valence orbital order of the first-year-lab diatomics, from ecce-qm.

    diatomic_order_test.py ECCE_QM_EXE BASIS_DIR     (ctest: qm_diatomic_order)

B3LYP/6-31G* at the experimental bond lengths of the Structure Library's
Teaching > Diatomics group.  Each orbital is classified from its MO
coefficients (sigma or pi from the weight on px,py; g or u from the inversion
parity of the coefficients), so the check is on the order of the symmetry
types and not on a degeneracy guess.  Exit 77 (skip) if ecce-qm is missing.
"""
import os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from molecules import MOLECULES  # noqa: E402

# Experimental r_e, Angstrom: NIST Chemistry WebBook, Constants of Diatomic
# Molecules (Huber & Herzberg 1979), rounded to 0.001; He2 is a convention.
# The same numbers are in data/client/StructureLibrary/Teaching/Diatomics.
REFERENCE = {"C2": ("C", "C", 1.243, 1), "N2": ("N", "N", 1.098, 1),
             "O2": ("O", "O", 1.208, 3), "CO": ("C", "O", 1.128, 1),
             "HF": ("H", "F", 0.917, 1), "He2": ("He", "He", 3.0, 1)}

# The valence order that must hold, lowest first (core orbitals are not listed).
EXPECTED = {
    "N2": ["2sg", "2su", "1pu", "3sg", "1pg", "3su"],
    "C2": ["2sg", "2su", "1pu", "3sg", "1pg", "3su"],
    "O2": ["2sg", "2su", "3sg", "1pu", "1pg", "3su"],
    "CO": ["3s", "4s", "1p", "5s", "2p", "6s"],
}

def run_qm(exe, basisdir, atoms, mult, workdir):
    inp = os.path.join(workdir, "in.txt")
    out = os.path.join(workdir, "out.txt")
    with open(inp, "w") as f:
        f.write("title d\ncharge 0\nmultiplicity %d\nmethod b3lyp\nbasis 6-31G*\ngeometry\n" % mult)
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % a)
        f.write("end\n")
    subprocess.run([exe, "--basis-dir", basisdir, "-o", out, inp], check=True,
                   stdout=subprocess.DEVNULL)
    energies, occ, labels, coefs = [], [], [], []
    section = None
    for line in open(out):
        t = line.split()
        if not t:
            continue
        if t[0] == "begin":
            section = t[1]
            continue
        if t[0] == "end":
            section = None
            continue
        if section == "orbitals":
            energies.append(float(t[1])); occ.append(float(t[2]))
        elif section == "ao_labels":
            labels.append(t[1:])
        elif section == "mo_coefficients":
            coefs.append([float(x) for x in t[1:]])
    return energies, occ, labels, coefs

def classify(labels, coefs, homonuclear):
    """['1sg', '1su', '2sg', ...]: s = sigma, p = pi; g/u only if homonuclear."""
    nbf = len(labels)
    half = nbf // 2
    count = {}
    names = []
    for c in coefs:
        pi = sum(c[i] ** 2 for i, l in enumerate(labels)
                 if l[1].endswith(("px", "py")) or l[1].endswith(("dxz", "dyz")))
        pi /= sum(x * x for x in c)
        kind = "p" if pi > 0.5 else "s"
        gu = ""
        if homonuclear:
            par = 0.0
            for i in range(half):
                l = 1 if ("p" in labels[i][1][1:]) else 0
                if labels[i][1][1:] in ("dxy", "dyz", "dz2", "dxz", "dx2-y2"):
                    l = 2
                par += (-1) ** l * c[i] * c[i + half]
            gu = "g" if par > 0 else "u"
        count[kind + gu] = count.get(kind + gu, 0) + 1
        if kind == "p":   # a degenerate pair is one level: name it once
            if count[kind + gu] % 2 == 0:
                names.append(names[-1])
                continue
            names.append("%d%s%s" % ((count[kind + gu] + 1) // 2, kind, gu))
        else:
            names.append("%d%s%s" % (count[kind + gu], kind, gu))
    return names

def analyse(exe, basisdir, name, dist, workdir):
    a, b, _, mult = REFERENCE[name]
    atoms = [(a, 0, 0, -dist / 2), (b, 0, 0, dist / 2)]
    e, occ, labels, coefs = run_qm(exe, basisdir, atoms, mult, workdir)
    return e, occ, classify(labels, coefs, a == b)

def main():
    if len(sys.argv) < 3 or not os.path.exists(sys.argv[1]):
        print("ecce-qm not built: skipped")
        return 77
    exe, basisdir = sys.argv[1], sys.argv[2]
    bad = 0
    with tempfile.TemporaryDirectory() as d:
        for name, want in EXPECTED.items():
            dist = REFERENCE[name][2]
            e, occ, names = analyse(exe, basisdir, name, dist, d)
            # Strip the core orbitals: the first len(names) - len(want) - extras
            # entries; find where the expected sequence starts.
            order = []
            for n in names:
                if n not in ("1sg", "1su", "1s", "2s") and (not order or order[-1] != n):
                    order.append(n)
            got = order[:len(want)]
            ok = got == want
            print("%-3s %s  %s" % (name, "ok  " if ok else "FAIL", " ".join(got)))
            if not ok:
                print("    expected", " ".join(want))
                bad += 1
            # HOMO/LUMO sanity: occupations must be filled from the bottom.
            if name == "O2":
                open_shell = [n for n, o in zip(names, occ) if abs(o - 1.0) < 1e-6]
                if open_shell != ["1pg", "1pg"]:
                    print("    FAIL O2 singly occupied orbitals:", open_shell)
                    bad += 1
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
