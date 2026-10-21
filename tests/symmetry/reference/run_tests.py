#!/usr/bin/env python3
"""Reference test for the symmetry helpers (#422).

    run_tests.py [--impl <dir or path prefix>] [--only autosym,genmol,...]
                 [--quiet]

Runs every case in corpus/ through the implementation and compares with
expected/ (made from the Fortran by generate_goldens.py).  --impl is a
directory holding autosym, cleansym, symops, genmol, getfrag, genmollat, or a
prefix such as /path/to/port-.  Default: the build directory's Fortran
binaries, which must pass everything.  Every difference is printed.

What is compared, and why (docs/claude/codereg/symmetry-helpers-interface.md):
  all        exit status exactly; on a non-zero status, stdout exactly.
  autosym    point group name exactly; coordinates in the output frame
             atom by atom, |d| <= TOL_XYZ.  The frame is part of the
             contract (the operations symops writes are in it).  A second,
             frame-free check (interatomic distances) tells a wrong frame
             from a wrong geometry.
  cleansym   coordinates atom by atom, |d| <= TOL_XYZ.
  symops     operation count; identity first; the remaining 3x3 matrices
             and translations as a SET (order is not used by consumers),
             |d| <= TOL_OP.
  getfrag    the list of kept atom numbers, exactly and in order.
  genmol     atom count, tags and coordinates in order, |d| <= TOL_XYZ.
  genmollat  as genmol, fractional coordinates, |d| <= TOL_FRAC.
"""
import argparse
import math
import os
import sys

import symref as S

TOL_XYZ = 1e-6     # A; outputs have 14 digits, iterative axis searches are not bitwise
TOL_OP = 1e-9      # operation matrices are tabulated, so effectively exact
TOL_FRAC = 1e-8    # fractional coordinates


def load_expected(prog, name):
    text = open(os.path.join(S.EXPECTED, prog, name + ".out")).read()
    head, _, body = text.partition("\n")
    return int(head.split()[1]), body


def dist_matrix(xyz):
    return [[math.dist(a, b) for b in xyz] for a in xyz]


def cmp_xyz(want, got, tol, label="atom"):
    d = []
    if len(want) != len(got):
        return ["%s count: expected %d, got %d" % (label, len(want), len(got))]
    for i, (w, g) in enumerate(zip(want, got)):
        m = max(abs(a - b) for a, b in zip(w, g))
        if m > tol:
            d.append("%s %d: expected %s got %s (max |d| %.3g > %g)" % (
                label, i + 1, ["%.8f" % v for v in w], ["%.8f" % v for v in g], m, tol))
    return d


def compare(prog, want_rc, want_out, rc, out):
    if rc != want_rc:
        return ["exit status: expected %d, got %d" % (want_rc, rc)]
    if want_rc != 0:
        return [] if out.strip() == want_out.strip() else [
            "stdout on failure differs: expected %r got %r" % (want_out.strip(), out.strip())]
    try:
        w, g = S.parse_output(prog, want_out), S.parse_output(prog, out)
    except Exception as e:
        return ["output not parseable: %s" % e]
    d = []
    if prog == "autosym":
        if w["group"].upper() != g["group"].upper():
            d.append("point group: expected %s got %s" % (w["group"], g["group"]))
        d += cmp_xyz(w["xyz"], g["xyz"], TOL_XYZ)
        if d and len(w["xyz"]) == len(g["xyz"]):
            dw, dg = dist_matrix(w["xyz"]), dist_matrix(g["xyz"])
            same = all(abs(a - b) <= TOL_XYZ for r, s in zip(dw, dg) for a, b in zip(r, s))
            d.append("geometry (interatomic distances) is %s" % (
                "the same, only the frame/orientation differs" if same else "also different"))
    elif prog == "cleansym":
        d += cmp_xyz(w["xyz"], g["xyz"], TOL_XYZ)
    elif prog == "getfrag":
        if w["idx"] != g["idx"]:
            d.append("kept atoms: expected %s got %s" % (w["idx"], g["idx"]))
    elif prog == "symops":
        wo, go = w["ops"], g["ops"]
        if len(wo) != len(go):
            return ["operation count: expected %d got %d" % (len(wo), len(go))]
        if wo and not all(abs(a - b) <= TOL_OP for r, s in zip(wo[0], go[0]) for a, b in zip(r, s)):
            d.append("the identity is not the first operation")
        left = list(range(len(go)))
        for i, m in enumerate(wo):
            hit = [j for j in left if all(abs(a - b) <= TOL_OP for r, s in zip(m, go[j])
                                           for a, b in zip(r, s))]
            if hit:
                left.remove(hit[0])
            else:
                d.append("expected operation %d is missing: %s" % (i, m))
        for j in left:
            d.append("unexpected operation: %s" % go[j])
    else:  # genmol, genmollat
        tol = TOL_FRAC if prog == "genmollat" else TOL_XYZ
        wa, ga = w["atoms"], g["atoms"]
        if len(wa) != len(ga):
            return ["atom count: expected %d got %d" % (len(wa), len(ga))]
        d += cmp_xyz([a[1] for a in wa], [a[1] for a in ga], tol)
        for i, (a, b) in enumerate(zip(wa, ga)):
            if a[0] != b[0]:
                d.append("atom %d tag: expected %s got %s" % (i + 1, a[0], b[0]))
        if d:
            key = lambda atoms: sorted((a[0],) + tuple(round(v, 5) for v in a[1]) for a in atoms)
            if key(wa) == key(ga):
                d.append("same atoms, different order")
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--impl", default=S.default_impl())
    ap.add_argument("--only", default="")
    ap.add_argument("--quiet", action="store_true", help="failed cases and the summary only")
    a = ap.parse_args()
    only = set(filter(None, a.only.split(",")))
    stats, failures = {}, 0
    for prog, name, path in S.all_cases():
        if only and prog not in only:
            continue
        want_rc, want_out = load_expected(prog, name)
        rc, out = S.run_case(a.impl, S.parse_case(path))
        diffs = compare(prog, want_rc, want_out, rc, out)
        t = stats.setdefault(prog, [0, 0])
        t[0] += 1
        if diffs:
            t[1] += 1
            failures += 1
            print("FAIL %s/%s" % (prog, name))
            for line in diffs:
                print("     " + line)
        elif not a.quiet:
            print("ok   %s/%s" % (prog, name))
    print()
    for prog in S.PROGRAMS:
        if prog in stats:
            print("%-10s %4d cases, %4d failed" % (prog, stats[prog][0], stats[prog][1]))
    print("TOTAL %d cases, %d failed" % (sum(v[0] for v in stats.values()), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
