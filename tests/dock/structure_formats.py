#!/usr/bin/env python3
"""Structure-file readers of the docking library (#1343).

  structure_formats.py native <dump> <fixtures>   no external program needed
  structure_formats.py oracle <dump> <fixtures>   vs Open Babel, 77 without it
  structure_formats.py bridge <dump> <fixtures>   Open Babel bridge, 77 without

The oracle is `obabel -omol2`, parsed here, never our own reader.  An aromatic
bond there matches order 1, 2 or our aromatic (4), since a Kekule file carries
no aromaticity; a non-aromatic bond must match exactly.
"""
import os
import shutil
import subprocess
import sys
import tempfile

mode, dump, fix = sys.argv[1:4]
fails = []


def check(cond, msg):
    if not cond:
        fails.append(msg)
        print("FAIL", msg)


def ours(path, typ, env=None):
    p = subprocess.run([dump, path, typ], capture_output=True, text=True, env=env)
    atoms, bonds = [], []
    for line in p.stdout.splitlines():
        w = line.split()
        if w[0] == "ERROR":
            return line, None
        if w[0] == "A":
            atoms.append((w[1], float(w[2]), float(w[3]), float(w[4]), float(w[5])))
        elif w[0] == "B":
            bonds.append((int(w[1]), int(w[2]), int(w[3])))
    return atoms, bonds


def obabel_mol2(path, extra=()):
    p = subprocess.run(["obabel"] + list(extra) + [path, "-omol2", "-l", "1"],
                       capture_output=True, text=True)
    atoms, bonds, sect = [], [], ""
    for line in p.stdout.splitlines():
        if line.startswith("@<TRIPOS>"):
            sect = line[9:].strip()
            continue
        w = line.split()
        if not w:
            continue
        if sect == "ATOM":
            atoms.append((w[5].split(".")[0], float(w[2]), float(w[3]), float(w[4])))
        elif sect == "BOND":
            t = w[3]
            o = {"1": 1, "2": 2, "3": 3, "ar": 4, "am": 1}.get(t, 1)
            bonds.append((int(w[1]) - 1, int(w[2]) - 1, o))
    return atoms, bonds


def key(b):
    return (min(b[0], b[1]), max(b[0], b[1]))


def compare(name, path, typ, obextra=(), perceived=False):
    a, b = ours(path, typ)
    check(b is not None, "%s: our reader failed: %s" % (name, a))
    if b is None:
        return
    oa, ob = obabel_mol2(path, obextra)
    check(len(a) == len(oa), "%s: %d atoms, obabel %d" % (name, len(a), len(oa)))
    if len(a) != len(oa):
        return
    for i, (x, y) in enumerate(zip(a, oa)):
        check(x[0] == y[0], "%s atom %d element %s vs %s" % (name, i, x[0], y[0]))
        d = max(abs(x[k] - y[k]) for k in (1, 2, 3))
        check(d < 1e-4, "%s atom %d coordinates differ by %g" % (name, i, d))
    od = {key(x): x[2] for x in ob}
    mine = {key(x): x[2] for x in b}
    check(set(od) == set(mine), "%s: bond pairs differ: ours-only %s obabel-only %s"
          % (name, sorted(set(mine) - set(od)), sorted(set(od) - set(mine))))
    for k in set(od) & set(mine):
        if od[k] == 4:
            ok = mine[k] in (1, 2, 4)
        elif perceived:
            # PDBQT has no bond orders; the reader finds only the short C=O
            # and C=N, so a ring double bond reads as single (the Builder
            # then applies its own distance-based orders).
            ok = mine[k] == od[k] or (mine[k] == 1 and od[k] == 2)
        else:
            ok = mine[k] == od[k]
        check(ok, "%s bond %s order %d, obabel %d" % (name, k, mine[k], od[k]))
    print("%s: %d atoms, %d bonds compared" % (name, len(a), len(b)))


def native():
    # Aromatic ligand, SDF: obabel wrote Kekule orders, so no order 4, and
    # the ring carries three double bonds.
    a, b = ours(fix + "/benzamide.sdf", "sdf")
    check(b is not None, "benzamide.sdf: %s" % (a,))
    if b is not None:
        check(len(a) == 16 and len(b) == 16, "benzamide.sdf counts %d/%d" % (len(a), len(b)))
        check(sorted(x[2] for x in b).count(2) == 4, "benzamide.sdf: 3 ring + 1 C=O double bonds")
    # The same ligand in MOL2: ring bonds are ar, the amide C-N is am (single).
    a, b = ours(fix + "/benzamide.mol2", "mol2")
    check(b is not None, "benzamide.mol2: %s" % (a,))
    if b is not None:
        check(len(a) == 16, "benzamide.mol2 atoms")
        check([x[2] for x in b].count(4) == 6, "benzamide.mol2: six aromatic bonds")
        check(sorted(set(x[0] for x in a)) == ["C", "H", "N", "O"], "benzamide.mol2 elements")
    # Several records: the first only.
    a, b = ours(fix + "/multi.sdf", "sdf")
    check(b is not None and len(a) == 11, "multi.sdf: first record (pyridine+H) %s" % (len(a) if b is not None else a,))
    # Vina poses: first MODEL, types A/OA/NA/HD/N/C mapped to elements.
    a, b = ours(fix + "/poses.pdbqt", "pdbqt")
    check(b is not None and len(a) == 11, "poses.pdbqt: first model has 11 atoms")
    if b is not None:
        check(sorted(set(x[0] for x in a)) == ["C", "H", "N", "O"], "poses.pdbqt elements")
        check(len(b) == 11, "poses.pdbqt bonds perceived")
    # Dummy atoms in MOL2 and macrocycle dummies in PDBQT.
    d = tempfile.mkdtemp()
    try:
        m = os.path.join(d, "dummy.mol2")
        open(m, "w").write("""@<TRIPOS>MOLECULE
x
 4 2 0 0 0
SMALL
USER_CHARGES

@<TRIPOS>ATOM
      1 C1          0.0000    0.0000    0.0000 C.3     1  UNL  -0.1000
      2 LP1         0.5000    0.0000    0.0000 LP      1  UNL   0.0000
      3 FE1         2.0000    0.0000    0.0000 Du      1  UNL   0.5000
      4 Cl          0.0000    1.7000    0.0000 Cl      1  UNL  -0.2000
@<TRIPOS>BOND
     1     1     2 1
     2     1     4 1
     3     1     3 nc
""")
        a, b = ours(m, "mol2")
        check(b is not None and [x[0] for x in a] == ["C", "Fe", "Cl"],
              "dummy.mol2 elements: %s" % (a,))
        check(b is not None and len(b) == 1 and abs(a[0][4] + 0.1) < 1e-6,
              "dummy.mol2: LP dropped with its bond, nc skipped, charge kept")
        q = os.path.join(d, "g.pdbqt")
        open(q, "w").write(
            "ATOM      1  C   UNL     1       0.000   0.000   0.000  0.00  0.00    +0.100 CG0\n"
            "ATOM      2  C   UNL     1       1.500   0.000   0.000  0.00  0.00    +0.000 G0\n"
            "ATOM      3  OA  UNL     1       2.700   0.000   0.000  0.00  0.00    -0.300 OA\n")
        a, b = ours(q, "pdbqt")
        check(b is not None and [x[0] for x in a] == ["C", "O"], "g.pdbqt: G0 dropped, CG0 is C: %s" % (a,))
            geometry()
        # V3000 without Open Babel says so.
        env = dict(os.environ, ECCE_OBABEL="/nonexistent/obabel", PATH="")
        a, b = ours(fix + "/acetic_v3000.mol", "mol", env)
        check(b is None and "V3000" in a and "Open Babel" in a, "V3000 error: %s" % (a,))
        # With obabel hidden, no bridge format is claimed.
        r = subprocess.run([dump, "--canread", "smi"], capture_output=True, text=True, env=env)
        check(r.stdout.strip() == "0", "canRead(smi) is false when obabel is hidden")
        a, b = ours(os.path.join(fix, "benzamide.sdf"), "smi", env)
        check(b is None, "smi is refused without obabel")
    finally:
        shutil.rmtree(d)


def geometry_cases():
    """Hand-built geometries: long real bonds survive, false ones go (#1344)."""
    return [
        # name, [(element, x, y, z)], expected bond pairs
        ("C-I", [("C", 0, 0, 0), ("I", 2.14, 0, 0), ("C", -0.5, 1.45, 0)],
         {(0, 1), (0, 2)}),
        ("S-S", [("S", 0, 0, 0), ("S", 2.05, 0, 0), ("C", -0.55, 1.73, 0),
                 ("C", 2.60, 1.73, 0)], {(0, 1), (0, 2), (1, 3)}),
        # P-O-P at 100 degrees: the P...P pair is inside the 0.45 A tolerance.
        ("P-O-P", [("P", 0, 0, 0), ("O", 1.6, 0, 0), ("P", 1.878, 1.576, 0)],
         {(0, 1), (1, 2)}),
        ("epoxide", [("C", 0, 0, 0), ("C", 1.47, 0, 0), ("O", 0.735, 1.227, 0)],
         {(0, 1), (0, 2), (1, 2)}),
        ("cyclopropane", [("C", 0, 0, 0), ("C", 1.51, 0, 0), ("C", 0.755, 1.308, 0)],
         {(0, 1), (0, 2), (1, 2)}),
    ]


def pdbqt_text(atoms):
    types = {"C": "C", "O": "OA", "S": "SA", "P": "P", "I": "I"}
    return "".join(
        "ATOM  %5d %-4s UNL     1    %8.3f%8.3f%8.3f  0.00  0.00    +0.000 %s\n"
        % (i + 1, " " + el, x, y, z, types[el])
        for i, (el, x, y, z) in enumerate(atoms))


def geometry():
    d = tempfile.mkdtemp()
    try:
        for name, atoms, want in geometry_cases():
            f = os.path.join(d, name + ".pdbqt")
            open(f, "w").write(pdbqt_text(atoms))
            a, b = ours(f, "pdbqt")
            check(b is not None and len(a) == len(atoms), "%s: reads: %s" % (name, a))
            if b is None:
                continue
            got = set(key(x) for x in b)
            check(got == want, "%s: bonds %s, want %s" % (name, sorted(got), sorted(want)))
            if shutil.which("obabel"):
                ob = set(key(x) for x in obabel_mol2(f)[1])
                check(ob == got, "%s: obabel has %s" % (name, sorted(ob)))
        print("geometry: %d hand-built cases" % len(geometry_cases()))
    finally:
        shutil.rmtree(d)


# Fragment-level orders (DockFragmentIO + generateBondOrders) against obabel's
# own perception of the same PDBQT.  Aromatic on either side matches 1, 1.5, 2.
# Known differences, (file, atom i, atom j): why.  generateBondOrders is
# shared code and is not changed here.
EXPECTED_FRAGMENT_DIFF = {}


def fragment():
    if not shutil.which("obabel"):
        print("obabel not found, fragment bond-order comparison skipped")
        sys.exit(77)
    r = fix + "/real/"
    cases = [(r + "aspirin_pubchem3d_ob.pdbqt", "aspirin_ob"),
             (r + "atp_rcsb_ob.pdbqt", "atp_ob"),
             (fix + "/poses.pdbqt", "poses")]
    for path, name in cases:
        p = subprocess.run([dump, "--dump", path, "PDBQT"], capture_output=True, text=True)
        bonds = {}
        n = 0
        for line in p.stdout.splitlines():
            w = line.split()
            if w[0] == "ERROR":
                check(False, "%s: %s" % (name, line))
            elif w[0] == "A":
                n += 1
            elif w[0] == "B":
                bonds[key((int(w[1]), int(w[2])))] = float(w[3])
        oa, ob = obabel_mol2(path)
        check(n == len(oa), "%s: %d atoms, obabel %d" % (name, n, len(oa)))
        raw = subprocess.run(["obabel", path, "-omol2", "-l", "1"], capture_output=True,
                             text=True).stdout
        ar = set()
        sect = ""
        for line in raw.splitlines():
            if line.startswith("@<TRIPOS>"):
                sect = line[9:].strip()
            elif sect == "BOND" and line.split()[3] == "ar":
                ar.add(key((int(line.split()[1]) - 1, int(line.split()[2]) - 1)))
        mine, theirs = set(bonds), set(key(x) for x in ob)
        check(mine == theirs, "%s: bond pairs differ: ours-only %s obabel-only %s"
              % (name, sorted(mine - theirs), sorted(theirs - mine)))
        nd = 0
        for x in ob:
            k = key(x)
            if k not in bonds:
                continue
            o, mo = x[2], bonds[k]
            ok = (mo == o) or (k in ar and mo in (1.0, 1.5, 2.0)) or \
                 (mo == 1.5 and o in (1, 2))
            if not ok:
                nd += 1
                why = EXPECTED_FRAGMENT_DIFF.get((name,) + k)
                print("%s %s bond %d-%d: Fragment order %.2f, obabel %d%s"
                      % ("EXPECTED" if why else "DIFF", name, k[0], k[1], mo, o,
                         " (" + why + ")" if why else ""))
                check(bool(why), "%s bond %s order %.2f vs obabel %d" % (name, k, mo, o))
        print("%s: %d bonds, %d order differences" % (name, len(bonds), nd))


REAL = ["aspirin_pubchem3d", "nirmatrelvir_pubchem2d", "atp_rcsb", "heme_rcsb"]


def oracle():
    if not shutil.which("obabel"):
        print("obabel not found, oracle comparison skipped")
        sys.exit(77)
    r = fix + "/real/"
    for n in REAL:
        compare(n + ".sdf", r + n + ".sdf", "sdf")
    for n in ("aspirin_pubchem3d", "atp_rcsb"):
        compare(n + ".mol2", r + n + ".mol2", "mol2")
    for n in ("aspirin_pubchem3d_ecce", "aspirin_pubchem3d_ob", "atp_rcsb_ecce",
              "atp_rcsb_ob", "heme_rcsb_ecce"):
        compare(n + ".pdbqt", r + n + ".pdbqt", "pdbqt", perceived=True)
    # A 2D file stays flat: no 3D regeneration behind the user's back.
    a, b = ours(r + "nirmatrelvir_pubchem2d.sdf", "sdf")
    check(b is not None and all(x[3] == 0.0 for x in a), "nirmatrelvir 2D: z must stay 0")
    compare("benzamide.sdf", fix + "/benzamide.sdf", "sdf")
    compare("benzamide.mol2", fix + "/benzamide.mol2", "mol2")
    compare("multi.sdf", fix + "/multi.sdf", "sdf")
    compare("poses.pdbqt", fix + "/poses.pdbqt", "pdbqt", perceived=True)
    # V3000 reaches us through the bridge; the oracle is obabel itself.
    compare("acetic_v3000.mol", fix + "/acetic_v3000.mol", "mol")


def bridge():
    if not shutil.which("obabel"):
        print("obabel not found, bridge test skipped")
        sys.exit(77)
    d = tempfile.mkdtemp()
    try:
        smi = os.path.join(d, "phenol.smi")
        open(smi, "w").write("c1ccccc1O phenol\n")
        r = subprocess.run([dump, "--canread", "smi"], capture_output=True, text=True)
        check(r.stdout.strip() == "1", "canRead(smi) with obabel")
        a, b = ours(smi, "smi")
        check(b is not None and len(a) == 13, "smi: 3D structure with hydrogens: %s" % (len(a) if b is not None else a,))
        if b is not None:
            check(any(abs(x[3]) > 1e-3 for x in a), "smi: not flat at the origin")
        cml = os.path.join(d, "m.cml")
        subprocess.run(["obabel", os.path.join(fix, "benzamide.sdf"), "-ocml", "-O", cml],
                       capture_output=True)
        compare("benzamide.cml", cml, "cml")
    finally:
        shutil.rmtree(d)


{"native": native, "oracle": oracle, "bridge": bridge, "geometry": geometry,
 "fragment": fragment}[mode]()
sys.exit(1 if fails else 0)
