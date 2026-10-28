#!/usr/bin/env python3
"""trace(P.S) = number of electrons, through scripts/parsers/xtb.mo.

The Molden files in fixtures/xtb are real xtb 6.7.1 --molden output (water,
methane, SO2 for the d functions).  What ECCE would see is what xtb.mo
prints (MO, ORBOCC, GTOBASIS), so that is what is checked: the overlap
matrix is built here, independently of ECCE and of xtb, from the primitives
of GTOBASIS (unnormalised, with every Cartesian d treated like xx as ECCE's
NORMP/NORMF do) in the order xtb.mo promises (atoms in order, shells by l;
p = x y z, d = xx yy zz xy xz yz), and the density matrix from the parsed
MOs.  A wrong order or normalisation gives a trace that is not N and
orbitals with c'Sc != 1.  Needs numpy only.
"""
import math
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
FIX = os.path.join(HERE, 'fixtures', 'xtb')
PARSER = os.path.join(HERE, '..', '..', 'scripts', 'parsers', 'xtb.mo')
CASES = [('h2o_gfn2.molden', 8), ('ch4_gfn2.molden', 8),
         ('so2_gfn2.molden', 18), ('ch3_uhf_gfn2.molden', 7)]
PWR = {'s': [(0, 0, 0)],
       'p': [(1, 0, 0), (0, 1, 0), (0, 0, 1)],
       'd': [(2, 0, 0), (0, 2, 0), (0, 0, 2), (1, 1, 0), (1, 0, 1),
             (0, 1, 1)]}


def read_atoms(path):
    atoms, sec = [], None
    for ln in open(path).read().splitlines():
        if ln.startswith('['):
            sec = ln.split()[0].lower()
            continue
        t = ln.split()
        if sec == '[atoms]' and len(t) >= 6:
            atoms.append((int(t[2]), [float(x) for x in t[3:6]]))
    return atoms


def parse_out(text):
    """key -> (size, values) of xtb.mo's output."""
    out = {}
    for blk in text.split('END\n'):
        ls = blk.strip().split('\n')
        if not ls or not ls[0].startswith('key: '):
            continue
        size = [int(x) for x in ls[ls.index('size:') + 1].split()]
        v = ls[ls.index('values:') + 1:]
        if 'units:' in v:
            v = v[:v.index('units:')]
        out[ls[0][5:]] = (size, v)
    return out


def floats(v):
    return [float(x) for x in ' '.join(v).split()]


def dfact(n):
    return 1.0 if n <= 0 else n * dfact(n - 2)


def s1d(i, j, a, b, d):
    """1D overlap of x^i exp(-a x^2) (at 0) and x^j exp(-b (x-d)^2)."""
    p = a + b
    pa = b * d / p
    pb = pa - d
    k = math.exp(-a * b / p * d * d) * math.sqrt(math.pi / p)
    tot = 0.0
    for m in range(i + 1):
        for n in range(j + 1):
            if (m + n) % 2:
                continue
            tot += (math.comb(i, m) * math.comb(j, n) * pa ** (i - m)
                    * pb ** (j - n) * dfact(m + n - 1)
                    / (2 * p) ** ((m + n) / 2))
    return k * tot


def ao_list(atoms, gto):
    """AOs in xtb.mo's promised order, from the parsed GTOBASIS rows."""
    rows = np.array(floats(gto[1])).reshape(gto[0])
    aos = []
    for z, xyz in atoms:
        r = rows[rows[:, 0] == z]
        for shell in sorted(set(r[:, 4])):
            sh = r[r[:, 4] == shell]
            kind = 'spd'[int(sh[0, 1])]
            prims = [(row[2], row[3]) for row in sh]
            for pw in PWR[kind]:
                aos.append((np.array(xyz), pw, prims))
    return aos


def overlap(aos):
    n = len(aos)
    S = np.zeros((n, n))
    for u in range(n):
        for v in range(n):
            ra, pa, ga = aos[u]
            rb, pb, gb = aos[v]
            d = rb - ra
            tot = 0.0
            for a, ca in ga:
                for b, cb in gb:
                    v3 = 1.0
                    for x in range(3):
                        v3 *= s1d(pa[x], pb[x], a, b, d[x])
                    tot += ca * cb * v3
            S[u, v] = tot
    return S


def main():
    bad = 0
    for name, nel in CASES:
        path = os.path.join(FIX, name)
        text = subprocess.run(
            ['perl', PARSER, '.', 'Energy', 'SE', 'GFN2-xTB'],
            input='ECCE-XTB-MOLDEN\n' + open(path).read(),
            capture_output=True, text=True, check=True).stdout
        o = parse_out(text)
        nmo, nbas = o['MO'][0]
        C = np.array(floats(o['MO'][1])).reshape(nmo, nbas).T
        occ = np.array(floats(o['ORBOCC'][1]))
        S = overlap(ao_list(read_atoms(path), o['GTOBASIS']))
        assert S.shape[0] == nbas, (S.shape, nbas)
        P = (C * occ) @ C.T
        tr = float(np.trace(P @ S))
        dev = float(abs(C.T @ S @ C - np.eye(nmo)).max())
        ok = abs(tr - nel) < 1e-4 and dev < 1e-4
        print("%s: nbas=%d  trace(PS)=%.6f (N=%d)  max|C'SC-1|=%.1e  %s"
              % (name, nbas, tr, nel, dev, 'ok' if ok else 'FAIL'))
        bad += not ok
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
