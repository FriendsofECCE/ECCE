#!/usr/bin/env python3
"""qm_scan: ecce-qm relaxed scans against ORCA 6.1.1 (scan_oracle.json), and
the keys the ECCE-QM parser makes of them (#450).

    scan_test.py ECCE_QM_EXE [--quick] [--table] [--filter REGEX]

Cases (scan_cases.py): ethane H-C-C-H torsion 60 to 180 degrees in 13 points
(HF/STO-3G, B3LYP/6-31G*), H2 bond 0.5 to 3.0 angstrom, RHF and UHF from a
broken-symmetry start.  Compared: every point's energy relative to the first
(0.1 kJ/mol), the barrier (0.1 kJ/mol); for the diatomics also the long-range
limit.  Independent of the oracle: each point's geometry holds the scanned
value, and the scan_table and scan_geometry blocks go through
scripts/parsers/ecceqm.parse into SCANCOORD, SCANENERGY and SCANGEOM.
"""
import json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from scan_cases import CASES, E_H, qm_input

REPO = os.path.dirname(os.path.dirname(HERE))
PARSE = os.path.join(REPO, "scripts", "parsers", "ecceqm.parse")
HARTREE_KJ = 2625.4996
TOL_KJ = 0.1


def run(exe, work, case, points=None):
    inp = os.path.join(work, "scan.qm")
    open(inp, "w").write(qm_input(case, points))
    p = subprocess.run([exe, inp], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("ecce-qm failed: " + (p.stdout[-300:] + p.stderr[-300:]))
    return p.stdout


def blocks(out, name):
    return re.findall(r"^begin %s\b[^\n]*\n(.*?)^end %s" % (name, name), out, re.S | re.M)


def coordinate(kind, atoms, xyz):
    """Bond (angstrom) or torsion (degrees, 0 to 360) of the 1-based atoms."""
    p = [xyz[a - 1] for a in atoms]
    if kind == "bond":
        return math.dist(p[0], p[1])
    b1 = [p[1][i] - p[0][i] for i in range(3)]
    b2 = [p[2][i] - p[1][i] for i in range(3)]
    b3 = [p[3][i] - p[2][i] for i in range(3)]
    cr = lambda u, v: [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
    dot = lambda u, v: sum(a * b for a, b in zip(u, v))
    n1, n2 = cr(b1, b2), cr(b2, b3)
    # IUPAC sign: positive when the far bond is turned clockwise seen along b2
    return math.degrees(math.atan2(math.sqrt(dot(b2, b2)) * dot(b1, n2), dot(n1, n2))) % 360.0


def parser_keys(out):
    """Feed the last scan_table and every scan_geometry block to ecceqm.parse."""
    res = {}
    tables = re.findall(r"^begin scan_table\b.*?^end scan_table\n", out, re.S | re.M)
    got = subprocess.run(["perl", PARSE], input=tables[-1], capture_output=True, text=True).stdout
    res["table"] = got
    frames = re.findall(r"^begin scan_geometry\b.*?^end scan_geometry\n", out, re.S | re.M)
    res["geom"] = [subprocess.run(["perl", PARSE], input=f, capture_output=True, text=True).stdout for f in frames]
    return res


def check_case(exe, name, table):
    case = CASES[name]
    oracle = json.load(open(os.path.join(HERE, "scan_oracle.json")))[name]
    with tempfile.TemporaryDirectory() as work:
        out = run(exe, work, case)
    kind, a, b, n = case["scan"]
    rows = [l.split() for l in blocks(out, "scan_table")[-1].splitlines()]
    val = [float(r[1]) for r in rows]
    en = [float(r[2]) for r in rows]
    fails = []
    if len(rows) != n or any(r[3] != "yes" for r in rows):
        fails.append("%d points, not all converged: %s" % (len(rows), [r[3] for r in rows]))
    ref = oracle["energy"]
    rel = lambda e: [(x - e[0]) * HARTREE_KJ for x in e]
    worst = max(abs(x - y) for x, y in zip(rel(en), rel(ref)))
    if worst > TOL_KJ:
        fails.append("relative energies differ from ORCA by up to %.3f kJ/mol" % worst)
    bar = lambda e: (max(e) - min(e)) * HARTREE_KJ
    if abs(bar(en) - bar(ref)) > TOL_KJ:
        fails.append("barrier %.3f against ORCA %.3f kJ/mol" % (bar(en), bar(ref)))
    # H2 at 3.0 angstrom: restricted orbitals stay far above two H atoms, the
    # broken-symmetry unrestricted ones reach them
    if case.get("limit") == "rhf" and en[-1] - 2 * E_H < 0.1:
        fails.append("RHF at the long end is %.4f Eh above 2 E(H), expected more than 0.1" % (en[-1] - 2 * E_H))
    if case.get("limit") == "uhf" and abs(en[-1] - 2 * E_H) > 1e-3:
        fails.append("UHF at the long end is %.4f Eh from 2 E(H)" % (en[-1] - 2 * E_H))
    if table:
        print("  %-12s %-16s %-16s %s" % ("value", "ecce-qm (Eh)", "ORCA (Eh)", "diff kJ/mol"))
        for v, e, r in zip(val, en, ref):
            print("  %-12.4f %-16.8f %-16.8f %+.4f" % (v, e, r, (e - r) * HARTREE_KJ))
    # the geometry of each point holds the scanned value
    geos = blocks(out, "scan_geometry")
    atoms = [int(x) for x in case["scan"][0].split()[1:]]
    for g, v in zip(geos, val):
        xyz = [[float(c) for c in l.split()[2:5]] for l in g.splitlines()]
        got = coordinate(kind.split()[0], atoms, xyz)
        want = v % 360.0 if kind.startswith("torsion") else v
        if abs(got - want) > (0.02 if kind.startswith("torsion") else 1e-4):
            fails.append("point at %g holds %g" % (v, got))
    if len(geos) != n:
        fails.append("%d scan_geometry blocks" % len(geos))
    # parser keys
    pk = parser_keys(out)
    m = re.search(r"key: SCANCOORD\nsize:\n(\d+)\nvalues:\n([^\n]*)\n(?:units:\n(\w+)\n)?", pk["table"])
    if not m or int(m.group(1)) != n or abs(float(m.group(2).split()[-1]) - val[-1]) > 1e-4:
        fails.append("SCANCOORD not read from the table")
    elif m.group(3) != ("Angstrom" if kind.startswith("bond") else "Degree"):
        fails.append("SCANCOORD unit %s" % m.group(3))
    m = re.search(r"key: SCANENERGY\nsize:\n(\d+)\nvalues:\n([^\n]*)\n", pk["table"])
    if not m or int(m.group(1)) != n or abs(float(m.group(2).split()[0]) - en[0]) > 1e-9:
        fails.append("SCANENERGY not read from the table")
    if len(pk["geom"]) != n or not all("key: SCANGEOM" in g and "size:\n1 %d 3" % len(case["atoms"]()) in g for g in pk["geom"]):
        fails.append("SCANGEOM not read from every scan_geometry block")
    return fails, en, ref, val


def main():
    args = sys.argv[1:]
    exe = args[0]
    quick = "--quick" in args
    table = "--table" in args
    pat = args[args.index("--filter") + 1] if "--filter" in args else ""
    bad = 0
    for name, case in CASES.items():
        if (quick and not case["quick"]) or not re.search(pat, name):
            continue
        fails, en, ref, val = check_case(exe, name, table)
        print("%-22s %s" % (name, "ok" if not fails else "FAIL"))
        for f in fails:
            print("   ", f)
        bad += bool(fails)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
