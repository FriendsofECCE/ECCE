#!/usr/bin/env python3
"""Make tests/qm/d3bj_orca.txt, the D3(BJ) oracle of qm_d3_test (#453).

    gen_d3bj_fixture.py <orca run dir> [--trim <dir>]

The run directory holds ORCA 6.1.1 single points "<mol>_<method>_d3" and
"<mol>_<method>_nod" (! <method> [D3BJ] def2-SVP VeryTightSCF EnGrad): the
dispersion energy is ORCA's printed Edisp, the dispersion gradient the
difference of the two .engrad gradients (same SCF otherwise).  --trim writes
the parts of the outputs the fixture is made from into <dir> (the copies kept
in tests/qm/d3bj_orca/); the script also runs on such a trimmed directory.
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
KEEP = (r"^\|\s+\d+> ", r"Edisp/kcal,au:", r"^Dispersion correction", r"FINAL SINGLE POINT ENERGY",
        r"Program Version")


def geometry(out):
    """Element symbols and coordinates (angstrom) from the input echo."""
    atoms, inside = [], False
    for line in out.splitlines():
        m = re.match(r"^\|\s+\d+>\s*(.*)$", line)
        if not m:
            continue
        t = m.group(1).split()
        if t[:2] == ["*", "xyz"]:
            inside = True
        elif inside and t[:1] == ["*"]:
            inside = False
        elif inside and len(t) == 4:
            atoms.append((t[0], [float(x) for x in t[1:]]))
    return atoms


def engrad(path):
    lines = [l.strip() for l in open(path) if not l.startswith("#")]
    n = int(lines[0])
    return [float(x) for x in lines[2:2 + 3 * n]]


def main():
    src = sys.argv[1]
    trim = sys.argv[sys.argv.index("--trim") + 1] if "--trim" in sys.argv else None
    rows = ["# D3(BJ) dispersion energy (hartree) and gradient (hartree/bohr) from ORCA 6.1.1,",
            "# made by gen_d3bj_fixture.py; atom lines: symbol x y z (angstrom) gx gy gz"]
    for d3 in sorted(glob.glob(os.path.join(src, "*_d3.out"))):
        name = os.path.basename(d3)[:-len("_d3.out")]
        mol, method = name.rsplit("_", 1)
        text = open(d3).read()
        edisp = float(re.search(r"Edisp/kcal,au:\s+\S+\s+(\S+)", text).group(1))
        g1 = engrad(os.path.join(src, name + "_d3.engrad"))
        g0 = engrad(os.path.join(src, name + "_nod.engrad"))
        atoms = geometry(text)
        assert len(g1) == len(g0) == 3 * len(atoms), name
        rows.append("case %s %s" % (mol, method))
        rows.append("energy %.12f" % edisp)
        for i, (sym, x) in enumerate(atoms):
            g = [g1[3 * i + c] - g0[3 * i + c] for c in range(3)]
            rows.append("atom %s %.6f %.6f %.6f %.12f %.12f %.12f" % (sym, *x, *g))
        if trim:
            os.makedirs(trim, exist_ok=True)
            kept = [l for l in text.splitlines() if any(re.search(k, l) for k in KEEP)]
            open(os.path.join(trim, name + "_d3.out"), "w").write("\n".join(kept) + "\n")
            for tag in ("_d3.engrad", "_nod.engrad"):
                open(os.path.join(trim, name + tag), "w").write(open(os.path.join(src, name + tag)).read())
    open(os.path.join(HERE, "d3bj_orca.txt"), "w").write("\n".join(rows) + "\n")


if __name__ == "__main__":
    main()
