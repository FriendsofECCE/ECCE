"""Build a minimally augmented basis (Zheng, Xu, Truhlar, Theor. Chem. Acc.
2011) from an ECCE .BAS file.

For every atom except H, one diffuse function per requested angular
momentum is added, with exponent = factor * the smallest exponent of that
angular momentum in the base set.  The BSE has no ma-def2-SVP, so the
library entry is derived from def2-SVP instead.

usage: make_ma_basis.py BASE.BAS OUT.BAS [--factor 0.3333333333333333] [--am SP]
"""
import argparse, re, sys

def parse(path):
    atoms, order = {}, []
    for line in open(path):
        line = line.rstrip("\n")
        if line.startswith("atom="):
            sym = line[5:].strip()
            atoms[sym] = []
            order.append(sym)
        elif line.startswith("contraction"):
            m = re.search(r"shell=(\w+) num_primitives=(\d+)", line)
            atoms[sym].append([m.group(1), line, []])
        elif line.strip():
            atoms[sym][-1][2].append(line)
    return order, atoms

def augment(shells, ams, factor):
    for am in ams:
        exps = [float(l.split()[0].replace("D", "E")) for sh in shells
                if sh[0] == am for l in sh[2]]
        if not exps:
            continue
        new = ["contraction shell=%s num_primitives=1 num_coefficients=1" % am,
               "%.10g 1.0000000" % (factor * min(exps))]
        idx = max(i for i, sh in enumerate(shells) if sh[0] == am) + 1
        shells.insert(idx, [am, new[0], [new[1]]])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base"); ap.add_argument("out")
    ap.add_argument("--factor", type=float, default=1.0 / 3.0)
    ap.add_argument("--am", default="SP")
    a = ap.parse_args()
    order, atoms = parse(a.base)
    with open(a.out, "w") as f:
        for sym in order:
            if sym != "H":
                augment(atoms[sym], a.am.upper(), a.factor)
            f.write("atom=%s\n" % sym)
            for _, head, prims in atoms[sym]:
                f.write(head + "\n")
                f.write("\n".join(prims) + "\n")

if __name__ == "__main__":
    main()
