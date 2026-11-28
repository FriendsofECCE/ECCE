#!/usr/bin/env python3
# Parses the Molden file xtb writes with --molden (gensub appends it to the
# output between ECCE-XTB-MOLDEN / ECCE-XTB-ENDMOLDEN) into MO, ORBENG, ORBOCC
# (and the *BETA keys for --uhf), MOAOORDER and GTOBASIS.
# What the file is, checked against an independent overlap matrix built from
# its own [GTO] block (tests/parsers/xtb_trace_ps_test.py):
#   * the MO coefficients multiply UNNORMALISED primitives r^l exp(-a r^2),
#     and the contracted functions come out unit-normalised;
#   * the basis is NOT orthonormal (STO-nG expansions of xtb's Slater basis),
#     which is why the MO diagram works for xtb and not for MOPAC;
#   * s, p are x y z; d is CARTESIAN (6 functions, no [5D] flag) in the order
#     xx yy zz xy xz yz, and xy/xz/yz are unit-normalised: Gaussian's
#     convention, which xTB.edml selects with componentNormalized="true".
# Columns are put in ECCE's canonical order (atoms in order, shells of one
# atom stably sorted by l), with no value scaling, and MOAOORDER says so.
import re
import sys
from xtb_parse import configure_streams, fields, number

configure_streams()
NF = {"s": 1, "p": 3, "d": 6}
LV = {"s": 0, "p": 1, "d": 2}
atom_z, shells, mos = [], [], []
section, atom = "", -1
lines = [line.rstrip("\n") for line in sys.stdin]
def missing_orbital(line):
    # Keep the failure contract when a Spin/Occup row has no preceding Ene.
    sys.stderr.write("Modification of non-creatable array value attempted, subscript -1 at {} line {}, <STDIN> line {}.\n".format(sys.argv[0], line, len(lines)))
    sys.exit(255)


i = 0
while i < len(lines):
    line = re.sub(r"D([-+]\d)", r"E\1", lines[i], flags=re.ASCII)
    i += 1
    m = re.search(r"^\s*\[(\w+)\]", line, re.ASCII)
    if m:
        section = m.group(1).lower()
        continue
    t = fields(line)
    if not t:
        continue
    if section == "atoms":
        if len(t) >= 6:
            atom_z.append(t[2])
    elif section == "gto":
        if len(t) == 2 and all(re.search(r"^\d+$", x, re.ASCII) for x in t):
            atom = number(t[0], integer=True) - 1
            while len(shells) <= atom:
                shells.append([])
            shells[atom] = []
        elif len(t) == 3 and t[0] in NF:
            prims = []
            count = number(t[1], integer=True)
            for k in range(max(0, count)):
                line = lines[i+k] if i+k < len(lines) else ""
                q = fields(re.sub(r"D([-+]\d)", r"E\1", line, flags=re.ASCII))
                prims.append((q + ["", ""])[:2])
            if not shells:
                shells.append([])
            shells[atom].append((t[0], prims))
            i += count
        elif len(t) == 3:
            sys.exit(0)
    elif section == "mo":
        m = re.search(r"^\s*Spin=\s*(\w+)", line, re.ASCII | re.I)
        e = re.search(r"^\s*Ene=\s*(\S+)", line, re.ASCII)
        o = re.search(r"^\s*Occup=\s*(\S+)", line, re.ASCII)
        if m:
            if not mos:
                missing_orbital(58)
            mos[-1]["spin"] = m.group(1).lower()
        elif e:
            mos.append(dict(ene=e.group(1), occ="0", spin="alpha", c=[]))
        elif o:
            if not mos:
                missing_orbital(60)
            mos[-1]["occ"] = o.group(1)
        elif len(t) == 2 and re.search(r"^\d+$", t[0], re.ASCII) and mos:
            mos[-1]["c"].append(t[1])
if not mos or not atom_z:
    sys.exit(0)
perm, col = [], 0
for atom_shells in shells:
    ordered = []
    for kind, prims in atom_shells:
        ordered.append((LV[kind], list(range(col, col + NF[kind]))))
        col += NF[kind]
    for _, columns in sorted(ordered, key=lambda s: s[0]):
        perm.extend(columns)
nbas = len(perm)
if any(len(m["c"]) != nbas for m in mos):
    sys.exit(0)


def labels(prefix, n, wrap):
    for j in range(1, n + 1):
        print("{}-{} ".format(prefix, j), end="")
        if j % wrap == 0:
            print()
    print()


def emit(mo_key, eng_key, occ_key, spin):
    selected = [m for m in mos if m["spin"] == spin]
    n = len(selected)
    if not n:
        return False
    for key, field, fmt, unit in ((eng_key, "ene", "%.8f", "Hartree"),
                                  (occ_key, "occ", "%.6f", "electrons")):
        print("key: {}\nsize:\n{}\nvalues:\n{}\nunits:\n{}\nEND".format(
            key, n, " ".join(fmt % number(m[field]) for m in selected), unit))
    print("key: {}\nsize:\n{} {}\nrowlabels:".format(mo_key, n, nbas))
    labels("MO", n, 8)
    print("columnlabels:")
    labels("BasFun", nbas, 6)
    print("values:")
    for m in selected:
        print(" ".join("%.10f" % (number(m["c"][j]) * 1.0) for j in perm))
    print("END")
    return True


if not emit("MO", "ORBENG", "ORBOCC", "alpha"):
    sys.exit(0)
emit("MOBETA", "ORBENGBETA", "ORBOCCBETA", "beta")
print("key: MOAOORDER\nsize:\n1\nvalues:\nangular-momentum\nEND")
seen, rows = set(), []
for at, atom_shells in enumerate(shells):
    z = atom_z[at] if at < len(atom_z) else ""
    if z in seen:
        continue
    seen.add(z)
    for sn, (kind, prims) in enumerate(sorted(atom_shells, key=lambda s: LV[s[0]]), 1):
        for exp, coef in prims:
            rows.append("%d %d %.12g %.12g %d" % (
                number(z, integer=True), LV[kind], number(exp), number(coef), sn))
print("key: GTOBASIS\nsize:\n{} 5\nrowlabels:".format(len(rows)))
labels("Primitive", len(rows), 8)
print("columnlabels:\nZ l Exponent Coefficient Shell\nvalues:")
for row in rows:
    print(row)
print("END")
