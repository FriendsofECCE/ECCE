#!/usr/bin/env python3
"""ecce-bandshift (#707): which band of calculation B is which band of A, and
how far it moved.

Wx-free; the Vibrations panel runs this with --json.

Usage: ecce-bandshift <calcA> <calcB> [--atoms SET] [--threshold T]
           [--json | --csv | --lines]

A and B are calculation folders (last GEOMTRACE frame, Props/VIB, VIBFREQ,
VIBIR) or modes files (read_modes_file).

The common atoms are all atoms of A when A and B have as many atoms, else
SET, 1-based and ordered: "1-12:5-16" maps A's atoms 1-12 to B's 5-16, a
set without a colon ("1-12") names the same indices in both.  B's geometry
is fitted onto A over the common atoms (proper rotation, Kabsch) and B's
modes are rotated the same way.  The stored vectors are taken as Cartesian
displacements of any normalisation; they are mass-weighted, cut to the common
atoms and renormalised, so an overlap is the cosine of the motion of the
common atoms alone.  Modes are matched one-to-one, greedily by largest
|overlap|, above --threshold (default 0.5).  The shift is B - A in cm-1.
"""
import argparse
import csv
import json
import math
import os
import re
import sys
import xml.etree.ElementTree as ET

_here = os.path.dirname(os.path.abspath(__file__))
try:
    sys.path.insert(0, _here)
    import ecce_displacement as D
except ImportError:
    # Installed as bin/ecce-displacement: no importable module name.
    import importlib.machinery
    import importlib.util
    _l = importlib.machinery.SourceFileLoader(
        "ecce_displacement", os.path.join(_here, "ecce-displacement"))
    _s = importlib.util.spec_from_loader("ecce_displacement", _l)
    D = importlib.util.module_from_spec(_s)
    _l.exec_module(D)

MASS = {
    "H": 1.008, "He": 4.0026, "Li": 6.94, "Be": 9.0122, "B": 10.81,
    "C": 12.011, "N": 14.007, "O": 15.999, "F": 18.998, "Ne": 20.180,
    "Na": 22.990, "Mg": 24.305, "Al": 26.982, "Si": 28.085, "P": 30.974,
    "S": 32.06, "Cl": 35.45, "Ar": 39.948, "K": 39.098, "Ca": 40.078,
    "Sc": 44.956, "Ti": 47.867, "V": 50.942, "Cr": 51.996, "Mn": 54.938,
    "Fe": 55.845, "Co": 58.933, "Ni": 58.693, "Cu": 63.546, "Zn": 65.38,
    "Ga": 69.723, "Ge": 72.630, "As": 74.922, "Se": 78.971, "Br": 79.904,
    "Kr": 83.798, "Rb": 85.468, "Sr": 87.62, "Y": 88.906, "Zr": 91.224,
    "Nb": 92.906, "Mo": 95.95, "Tc": 98.0, "Ru": 101.07, "Rh": 102.91,
    "Pd": 106.42, "Ag": 107.87, "Cd": 112.41, "In": 114.82, "Sn": 118.71,
    "Sb": 121.76, "Te": 127.60, "I": 126.90, "Xe": 131.29, "Cs": 132.91,
    "Ba": 137.33, "La": 138.91, "Hf": 178.49, "Ta": 180.95, "W": 183.84,
    "Re": 186.21, "Os": 190.23, "Ir": 192.22, "Pt": 195.08, "Au": 196.97,
    "Hg": 200.59, "Tl": 204.38, "Pb": 207.2, "Bi": 208.98,
}


def _vector(path):
    """The numbers of a <vector> property file."""
    root = ET.parse(path).getroot()
    return [float(x) for x in (root.text or "").split()]


def read_modes_file(path):
    """The panel's hand-over for calculations it cannot read from disk: N, N
    lines "El x y z", "modes M" and per mode a frequency and N lines
    "dx dy dz" (as ecce-displace's), then optionally "ir M" and M values."""
    tok = open(path).read().split()
    n = int(tok[0])
    pos = 1
    els, xyz = [], []
    for _ in range(n):
        els.append(tok[pos])
        xyz.append(tuple(float(t) for t in tok[pos + 1:pos + 4]))
        pos += 4
    freqs, modes, ir = [], [], None
    while pos < len(tok):
        word = tok[pos]
        pos += 1
        if word == "modes":
            for _ in range(int(tok[pos])):
                freqs.append(float(tok[pos + 1]))
                v = [float(t) for t in tok[pos + 2:pos + 2 + 3 * n]]
                modes.append([v[3 * a:3 * a + 3] for a in range(n)])
                pos += 1 + 3 * n
            pos += 1
        elif word == "ir":
            m = int(tok[pos])
            ir = [float(t) for t in tok[pos + 1:pos + 1 + m]]
            pos += 1 + m
    if ir is not None and len(ir) != len(freqs):
        ir = None
    return els, xyz, freqs, ir, modes


def read_modes(calc):
    """-> (elements, coords, freqs, ir or None, modes [M][N][3]) of the last
    geometry and the stored VIB of a calculation folder (or a modes file)."""
    if os.path.isfile(calc):
        return read_modes_file(calc)
    elements, coords = D.final_geometry(calc)
    if any(e is None for e in elements):
        raise ValueError("%s: the elements of the atoms are unknown" % calc)
    props = os.path.join(calc, "Props")
    try:
        freqs = _vector(os.path.join(props, "VIBFREQ"))
        root = ET.parse(os.path.join(props, "VIB")).getroot()
    except (OSError, ET.ParseError):
        raise ValueError("%s has no stored normal modes (Props/VIBFREQ, VIB)"
                         % calc)
    ir = None
    if os.path.isfile(os.path.join(props, "VIBIR")):
        ir = _vector(os.path.join(props, "VIBIR"))
        if len(ir) != len(freqs):
            ir = None
    n = len(coords)
    modes = []
    for m in root:
        if m.tag != "matrix":
            continue
        v = [float(x) for x in (m.text or "").split()]
        if len(v) != 3 * n:
            raise ValueError("%s: the normal modes do not match the "
                             "structure (%d atoms)" % (calc, n))
        modes.append([v[3 * a:3 * a + 3] for a in range(n)])
    if len(modes) != len(freqs):
        raise ValueError("%s: %d frequencies but %d modes"
                         % (calc, len(freqs), len(modes)))
    return elements, coords, freqs, ir, modes


def parse_set(text, n):
    """'1-3,5' -> [0, 1, 2, 4] (1-based in, 0-based out, order kept)."""
    out = []
    for tok in re.split(r"[\s,]+", text.strip()):
        if not tok:
            continue
        m = re.match(r"^(\d+)(?:-(\d+))?$", tok)
        if not m:
            raise ValueError("bad atom set '%s'" % text)
        lo = int(m.group(1))
        hi = int(m.group(2) or lo)
        if lo < 1 or hi < lo or hi > n:
            raise ValueError("atoms %s are out of range (1..%d)" % (tok, n))
        out.extend(range(lo - 1, hi))
    return out


def common_atoms(spec, na, nb):
    """-> (indices in A, indices in B), pairwise."""
    if not spec:
        if na != nb:
            raise ValueError("A has %d atoms and B %d: give --atoms" % (na, nb))
        return list(range(na)), list(range(nb))
    if ":" in spec:
        sa, sb = spec.split(":", 1)
        ia, ib = parse_set(sa, na), parse_set(sb, nb)
    else:
        ia = parse_set(spec, min(na, nb))
        ib = list(ia)
    if len(ia) != len(ib):
        raise ValueError("the two atom sets have %d and %d atoms"
                         % (len(ia), len(ib)))
    if len(ia) < 3:
        raise ValueError("at least 3 common atoms are needed for the fit")
    return ia, ib


def _weighted(mode, idx, rot, masses):
    """The mode's motion of atoms idx, optionally rotated, mass-weighted and
    normalised; None when it does not move them."""
    v = []
    for a in idx:
        d = mode[a]
        if rot:
            d = [sum(rot[i][k] * d[k] for k in range(3)) for i in range(3)]
        w = math.sqrt(masses[a])
        v.extend(c * w for c in d)
    nrm = math.sqrt(sum(c * c for c in v))
    if nrm < 1e-10:
        return None
    return [c / nrm for c in v]


def bandshift(a, b, atoms=None, threshold=0.5):
    """a, b: read_modes() results.  -> dict, see the module docstring."""
    ela, xa, fa, ira, ma = a
    elb, xb, fb, irb, mb = b
    ia, ib = common_atoms(atoms, len(xa), len(xb))
    rmsd, _al, rot = D.kabsch_rotation([xa[i] for i in ia],
                                       [xb[i] for i in ib])
    massa = [MASS.get(e) for e in ela]
    massb = [MASS.get(e) for e in elb]
    if any(massa[i] is None for i in ia) or any(massb[i] is None for i in ib):
        raise ValueError("an element of the common atoms has no known mass")
    va = [_weighted(m, ia, None, massa) for m in ma]
    vb = []
    for m in mb:
        sub = [m[i] for i in ib]
        vb.append(_weighted(sub, range(len(ib)), rot,
                            [massb[i] for i in ib]))
    cand = []
    for i, x in enumerate(va):
        if x is None:
            continue
        for j, y in enumerate(vb):
            if y is None:
                continue
            o = abs(sum(p * q for p, q in zip(x, y)))
            if o >= threshold:
                cand.append((-o, i, j))
    cand.sort()
    used_a, used_b, pairs = set(), set(), []
    for no, i, j in cand:
        if i in used_a or j in used_b:
            continue
        used_a.add(i)
        used_b.add(j)
        pairs.append((i, j, -no))
    pairs.sort(key=lambda p: fa[p[0]])

    def band(freqs, ir, i):
        return {"mode": i + 1, "freq": freqs[i],
                "ir": ir[i] if ir else None}

    matches = []
    for i, j, o in pairs:
        ba, bb = band(fa, ira, i), band(fb, irb, j)
        matches.append({"mode_a": ba["mode"], "freq_a": ba["freq"],
                        "ir_a": ba["ir"], "mode_b": bb["mode"],
                        "freq_b": bb["freq"], "ir_b": bb["ir"],
                        "shift": bb["freq"] - ba["freq"], "overlap": o})

    def rest(freqs, ir, used, vecs):
        out = []
        for i in range(len(freqs)):
            if i in used:
                continue
            r = band(freqs, ir, i)
            r["reason"] = ("no motion of the common atoms"
                           if vecs[i] is None else
                           "overlap below %g" % threshold)
            out.append(r)
        return out

    return {"n_common": len(ia), "fit_rmsd": rmsd, "threshold": threshold,
            "matches": matches,
            "unmatched_a": rest(fa, ira, used_a, va),
            "unmatched_b": rest(fb, irb, used_b, vb)}


COLS = ["mode_a", "freq_a", "ir_a", "mode_b", "freq_b", "ir_b", "shift",
        "overlap"]


def _num(v, fmt):
    return "-" if v is None else fmt % v


def as_text(res, na, nb):
    out = ["A: %s   B: %s   common atoms: %d (fit RMSD %.3f A)   "
           "overlap threshold %g" % (na, nb, res["n_common"],
                                    res["fit_rmsd"], res["threshold"]),
           "%6s %9s %9s  %6s %9s %9s  %8s %7s" %
           ("mode A", "freq A", "IR A", "mode B", "freq B", "IR B",
            "shift", "overlap")]
    for m in res["matches"]:
        out.append("%6d %9.1f %9s  %6d %9.1f %9s  %+8.1f %7.3f" % (
            m["mode_a"], m["freq_a"], _num(m["ir_a"], "%.1f"), m["mode_b"],
            m["freq_b"], _num(m["ir_b"], "%.1f"), m["shift"], m["overlap"]))
    for key, name in (("unmatched_a", "A"), ("unmatched_b", "B")):
        if res[key]:
            out.append("Unmatched in %s:" % name)
            for r in res[key]:
                out.append("  mode %d  %.1f cm-1  IR %s  (%s)" % (
                    r["mode"], r["freq"], _num(r["ir"], "%.1f"), r["reason"]))
    return "\n".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("calc_a")
    ap.add_argument("calc_b")
    ap.add_argument("--atoms", default="",
                    help="common atoms: '1-12' or '1-12:5-16' (A:B)")
    ap.add_argument("--threshold", type=float, default=0.5)
    ap.add_argument("--lines", action="store_true",
                    help="tab-separated lines for the GUI")
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--json", action="store_true")
    g.add_argument("--csv", action="store_true")
    o = ap.parse_args(argv)
    try:
        res = bandshift(read_modes(o.calc_a), read_modes(o.calc_b),
                        o.atoms, o.threshold)
    except ValueError as e:
        print("ecce-bandshift: %s" % e, file=sys.stderr)
        return 1
    na = os.path.basename(os.path.normpath(o.calc_a))
    nb = os.path.basename(os.path.normpath(o.calc_b))
    if o.lines:
        print("header\t%d\t%.6f\t%g" % (res["n_common"], res["fit_rmsd"],
                                          res["threshold"]))
        for m in res["matches"]:
            print("match\t%d\t%.4f\t%s\t%d\t%.4f\t%s\t%.4f\t%.6f" % (
                m["mode_a"], m["freq_a"], _num(m["ir_a"], "%.4f"),
                m["mode_b"], m["freq_b"], _num(m["ir_b"], "%.4f"),
                m["shift"], m["overlap"]))
        for key, tag in (("unmatched_a", "A"), ("unmatched_b", "B")):
            for r in res[key]:
                print("unmatched\t%s\t%d\t%.4f\t%s\t%s" % (
                    tag, r["mode"], r["freq"], _num(r["ir"], "%.4f"),
                    r["reason"]))
    elif o.json:
        res["a"], res["b"] = na, nb
        json.dump(res, sys.stdout)
        print()
    elif o.csv:
        w = csv.writer(sys.stdout, lineterminator="\n")
        w.writerow(COLS)
        for m in res["matches"]:
            w.writerow([m[c] if m[c] is not None else "" for c in COLS])
    else:
        print(as_text(res, na, nb))
    return 0


if __name__ == "__main__":
    sys.exit(main())
