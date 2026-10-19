#!/usr/bin/env python3
"""Textbook checks over the phase sketches of the ligand sets (#368).

For Cr(CO)6 (Oh) and Ni(CO)4 / CoCl4(2-) (Td) the sign pattern the sketch
holds for each ligand set is compared with the group-theoretical form of that
irrep, built here from the donor positions in the dump and nothing else:

  sigma (one number per donor, s and the p lobe facing the centre)
    a1g, a1   all equal
    eg        trans partners equal, sum zero (z2 and x2-y2 patterns)
    t1u, t2   v_i = u . r_i  (a translation / (x, y, z) pattern)
  pi (p vector across the bond at each donor)
    t2g       p_i = grad f(r_i), f in span(xy, yz, zx)
    t1u       p_i = the component of a fixed vector u across r_i

A degenerate set's first orbital is any combination inside its irrep, so each
test is "lies in the irrep's space", fitted by least squares.
Run: python3 tests/modiagram/check_sketch.py [dir-with-dumps]
"""
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
TOL = 0.06


def right_column(path):
    """(points, [(label, irrep, phases)]) of the right-hand column."""
    points, levels, inside = None, [], False
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column right"
        elif inside and line.lstrip().startswith("level "):
            m = re.search(r"phases=\[([^\]]*)\]", line)
            if not m:
                continue
            irrep = re.search(r"irrep=(\S+)", line).group(1)
            label = re.search(r"label=(\S+)", line).group(1)
            levels.append((label, irrep, np.array([float(x) for x in m.group(1).split(",")])))
        elif inside and line.lstrip().startswith("sketch "):
            m = re.search(r"points=\[([^\]]*)\]", line)
            view = int(re.search(r"view=(\d+)", line).group(1))
            if view == 1:
                points = np.array([[float(x) for x in p.split(",")] for p in m.group(1).split(";")])
    return points, levels


def residual(basis, target):
    """Relative distance of `target` from the span of the columns of `basis`."""
    n = np.linalg.norm(target)
    if n < 1e-9:
        return 1.0
    coef, *_ = np.linalg.lstsq(basis, target, rcond=None)
    return np.linalg.norm(basis @ coef - target)/n


def partner(points, i):
    for j in range(len(points)):
        if np.allclose(points[j], -points[i], atol=1e-3):
            return j
    return None


def sigma_a(points, v):
    return np.ptp(v) < TOL*np.abs(v).max() and abs(v.mean()) > 0.5*np.abs(v).max()


def sigma_eg(points, v):
    pairs = [(i, partner(points, i)) for i in range(len(points))]
    if any(j is None for _, j in pairs):
        return False
    even = all(abs(v[i] - v[j]) < TOL*np.abs(v).max() for i, j in pairs)
    return even and abs(v.sum()) < TOL*np.abs(v).max()*len(v) and np.abs(v).max() > 0


def sigma_t(points, v):
    return residual(points, v) < TOL


def pi_t2g(points, v):
    cols = []
    for f in range(3):
        c = np.zeros(3*len(points))
        for i, (x, y, z) in enumerate(points):
            c[3*i:3*i+3] = [(y, x, 0), (0, z, y), (z, 0, x)][f]
        cols.append(c)
    return residual(np.array(cols).T, v) < TOL


def pi_t1u(points, v):
    cols = []
    for k in range(3):
        c = np.zeros(3*len(points))
        for i, r in enumerate(points):
            u = np.zeros(3)
            u[k] = 1.0
            rh = r/np.linalg.norm(r)
            c[3*i:3*i+3] = u - (u @ rh)*rh
        cols.append(c)
    return residual(np.array(cols).T, v) < TOL


#  (irrep, sigma?) -> test, per group.  Every listed case must be found.
RULES = {
    #  The default diagram draws the CO sigma sets and t2g; the t1u pi set only
    #  has a line to the molecule with every level shown.
    "orca-crco6": {("A1G", True): sigma_a, ("EG", True): sigma_eg, ("T1U", True): sigma_t,
                   ("T2G", False): pi_t2g},
    "orca-crco6-all": {("A1G", True): sigma_a, ("EG", True): sigma_eg, ("T1U", True): sigma_t,
                       ("T2G", False): pi_t2g, ("T1U", False): pi_t1u},
    "orca-nico4": {("A1", True): sigma_a, ("T2", True): sigma_t},
    "orca-cocl4-quartet": {("A1", True): sigma_a, ("T2", True): sigma_t},
}


def check(d):
    bad = 0
    for name, rules in RULES.items():
        path = os.path.join(d, name + ".dump")
        if not os.path.exists(path):
            print("%-20s no dump" % name)
            bad += 1
            continue
        points, levels = right_column(path)
        if points is None:
            print("%-20s FAIL: no oblique sketch" % name)
            bad += 1
            continue
        n = len(points)
        seen = set()
        for label, irrep, v in levels:
            sigma = len(v) == n
            if not sigma and len(v) != 3*n:
                print("%-20s %s: %d numbers for %d donors" % (name, label, len(v), n))
                bad += 1
                continue
            rule = rules.get((irrep, sigma))
            if rule is None:
                continue
            ok = bool(rule(points, v))
            seen.add((irrep, sigma))
            bad += not ok
            print("%-20s %-4s %-5s %s" % (name, irrep, "sigma" if sigma else "pi", "ok" if ok else "FAIL"))
        for key in rules:
            if key not in seen:
                bad += 1
                print("%-20s %s %s: no such sketch" % (name, key[0], "sigma" if key[1] else "pi"))
    print("phase sketches against the textbook patterns: %s" % ("FAIL" if bad else "ok"))
    return bad


if __name__ == "__main__":
    sys.exit(1 if check(sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "expected")) else 0)
