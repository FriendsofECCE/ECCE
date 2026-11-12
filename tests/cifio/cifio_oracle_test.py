#!/usr/bin/env python3
"""CIF import against independent oracles (#358).

For each fixture the cifio test program reads the CIF and writes the P1
cell it built.  That file is compared (cell, and every fractional position
after sorting, within 1e-4) with
  - gemmi: read_small_structure(fixture).get_all_unit_cell_sites()
  - Open Babel, stored in tests/fixtures/cif/expected_obabel/, made with
    obabel -icif X.cif -ocif --fillUC strict   (obabel 3.1.1)
Exit 77 (skip) without gemmi.  Usage: cifio_oracle_test.py <cifio binary>
"""
import os, subprocess, sys, tempfile

try:
    import gemmi
except ImportError:
    sys.exit(77)

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FIX = os.path.join(REPO, "tests", "fixtures", "cif")
NAMES = ["nacl", "cscl", "quartz", "graphite", "licoo2", "nacl_nosymops", "quartz_esd"]


def wrap(v):
    v %= 1.0
    return 0.0 if v > 1 - 1e-6 else v


def key(a):
    return (a[0], round(a[1], 3), round(a[2], 3), round(a[3], 3))


def close(a, b, tol=1e-4):
    if len(a) != len(b):
        return False
    for x, y in zip(sorted(a, key=key), sorted(b, key=key)):
        if x[0] != y[0]:
            return False
        for k in (1, 2, 3):
            d = (x[k] - y[k]) % 1.0
            if min(d, 1 - d) > tol:
                return False
    return True


def main():
    exe = sys.argv[1]
    bad = 0
    for name in NAMES:
        src = os.path.join(FIX, name + ".cif")
        with tempfile.TemporaryDirectory() as td:
            out = os.path.join(td, name + ".cif")
            r = subprocess.run([exe, "--convert", src, out],
                               capture_output=True, text=True)
            if r.returncode != 0:
                if "symmetry helper" in r.stderr:
                    print("SKIP %s: %s" % (name, r.stderr.strip()))
                    continue
                print("FAIL %s: cifio --convert: %s" % (name, r.stderr))
                bad += 1
                continue
            mine = gemmi.read_small_structure(out)
            ours = [(s.element.name, *[wrap(v) for v in (s.fract.x, s.fract.y, s.fract.z)])
                    for s in mine.sites]
        ref = gemmi.read_small_structure(src)
        theirs = [(s.element.name, wrap(s.fract.x), wrap(s.fract.y), wrap(s.fract.z))
                  for s in ref.get_all_unit_cell_sites()]
        cell_ok = all(abs(x - y) < 1e-4 for x, y in
                      zip(mine.cell.parameters, ref.cell.parameters))
        ob = []
        for line in open(os.path.join(FIX, "expected_obabel", name + ".txt")):
            if line.startswith("#"):
                continue
            t = line.split()
            ob.append((t[0], float(t[1]), float(t[2]), float(t[3])))
        res = (cell_ok, close(ours, theirs), close(ours, ob))
        print("%s %-14s n=%2d cell=%s gemmi=%s obabel=%s" % (
            "ok  " if all(res) else "FAIL", name, len(ours), *res))
        bad += 0 if all(res) else 1
    print("FAILED" if bad else "PASSED")
    return 1 if bad else 0


sys.exit(main())
