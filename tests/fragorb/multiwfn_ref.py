#!/usr/bin/env python3
"""Reference projections from Multiwfn's charge decomposition analysis.

    multiwfn_ref.py MULTIWFN PARENT.molden FRAG1.molden ... > crco6_gfo_*.tsv

Multiwfn (main function 16) takes fragment wavefunctions in the parent's
basis, builds the fragment orbitals' overlap matrix with its own integrals
and expands every complex orbital in them (options 3 and 4 export both).
From those two matrices, with no code of ours involved, this derives the
quantity FragmentOrbitals computes: the weight of each complex orbital on
the symmetrically orthogonalised fragment orbitals,

    w_k = | (S_FF^1/2 c)_k |^2        (c = Multiwfn's coefficient matrix)

and prints, per complex orbital, the sum over all FOs (capture) and over
each fragment.  With `--occupied N0 N1 ...` only the first N_i orbitals of
fragment i are used (b = S c are the overlaps with the complex orbital,
w = | S_sub^-1/2 b_sub |^2).

Fragments must be consecutive blocks of the parent's atoms, in order
(Multiwfn requires it).  Needs numpy; Multiwfn is not part of the repo.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

import numpy as np


def blocks(path):
    rows, hdr = {}, []
    for line in open(path):
        t = line.split()
        if not t or not t[0].isdigit():
            continue
        if all(x.isdigit() for x in t):
            hdr = [int(x) for x in t]
            continue
        if len(t) < 2 or "." not in t[1]:
            continue
        rows.setdefault(int(t[0]), {})
        for c, v in zip(hdr, t[1:]):
            rows[int(t[0])][c] = float(v)
    n = max(rows)
    m = max(max(r) for r in rows.values())
    a = np.zeros((n, m))
    for i, r in rows.items():
        for c, v in r.items():
            a[i - 1, c - 1] = v
    return a


def main():
    args = sys.argv[1:]
    occ = None
    if "--occupied" in args:
        i = args.index("--occupied")
        occ = [int(x) for x in args[i + 1:]]
        args = args[:i]
    mw, parent, frags = args[0], args[1], args[2:]
    work = tempfile.mkdtemp(prefix="fragorb-mw-")
    ab = [os.path.abspath(x) for x in [parent] + frags]
    script = "\n".join([ab[0], "16", str(len(frags))] + ab[1:] +
                       ["3", "4", "-1", "q", ""])
    env = dict(os.environ, Multiwfnpath=os.path.dirname(os.path.abspath(mw)),
               OMP_STACKSIZE="200M")
    out = subprocess.run([mw], input=script, cwd=work, env=env, text=True,
                         capture_output=True).stdout
    sizes = [int(m) - int(l) + 1 for l, m in
             re.findall(r"FO from\s+(\d+) to\s+(\d+) correspond", out)[:len(frags)]]
    c = blocks(os.path.join(work, "coFO.txt"))
    s = blocks(os.path.join(work, "ovlpint.txt"))
    edges = np.concatenate([[0], np.cumsum(sizes)])
    if occ is None:
        keep = list(range(s.shape[0]))
        ev, u = np.linalg.eigh(s)
        w = ((u @ np.diag(np.sqrt(ev)) @ u.T) @ c) ** 2
    else:
        keep = []
        for f, n in enumerate(occ):
            keep += list(range(edges[f], edges[f] + n))
        ev, u = np.linalg.eigh(s[np.ix_(keep, keep)])
        w = ((u @ np.diag(1 / np.sqrt(ev)) @ u.T) @ (s @ c)[keep]) ** 2
    shutil.rmtree(work, ignore_errors=True)
    print("# Multiwfn CDA matrices -> w_k = |S_FF^(+-1/2) b|^2; columns: MO capture, per fragment")
    for m in range(w.shape[1]):
        per = []
        for f in range(len(frags)):
            idx = [i for i, k in enumerate(keep) if edges[f] <= k < edges[f + 1]]
            per.append(w[idx, m].sum())
        print("%d\t%.5f\t%s" % (m + 1, w[:, m].sum(), "\t".join("%.5f" % x for x in per)))


main()
