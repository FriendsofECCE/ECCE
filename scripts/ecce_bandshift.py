#!/usr/bin/env python3
"""ecce-bandshift (#707): which band of calculation B is which band of A, and
how far it moved.

Wx-free; the Vibrations panel runs this with --json.

Usage: ecce-bandshift <calcA> <calcB> [--atoms SET] [--threshold T]
           [--json | --csv | --lines]

A and B are calculation folders (last GEOMTRACE frame, Props/VIB, VIBFREQ,
VIBIR) or modes files (read_modes_file).

The common atoms are found automatically: the smaller calculation (the free
molecule) is looked up in the larger one as a bonded unit, by element and
covalent bonds (bonds to metal atoms do not count, so a bound anion or ligand
is a unit of its own).  Every copy found gives one map; among the equivalent
maps of a copy (the O atoms of a sulfate, the H atoms of a methyl) the one with
the lowest Kabsch RMSD is kept.  --atoms SET overrides this for scripts:
1-based and ordered, "1-12:5-16" maps A's atoms 1-12 to B's 5-16, a set
without a colon names the same indices in both; mapped elements must agree.

The free molecule's geometry is fitted onto each copy over the common atoms
(proper rotation, Kabsch) and the modes are rotated the same way.  The stored
vectors are taken as Cartesian displacements of any normalisation; they are
mass-weighted.  The overlap is the free molecule's motion against the larger
calculation's mode cut to the common atoms, divided by that mode's length over
ALL its atoms: a mode that hardly moves the common atoms scores low however
alike the shapes are.  With one copy, modes are
matched one-to-one, greedily by largest |overlap|, above --threshold (default
0.5).  With several copies every band of the larger calculation takes the free
band it resembles most in any phase of the copies, so one free band may pair
with several (in-phase and out-of-phase combinations): a split.  The shift is
B - A in cm-1.
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


# Covalent radii in A (Cordero 2008); unknown elements get 1.5.
RADIUS = {
    "H": 0.31, "He": 0.28, "Li": 1.28, "Be": 0.96, "B": 0.84, "C": 0.76,
    "N": 0.71, "O": 0.66, "F": 0.57, "Ne": 0.58, "Na": 1.66, "Mg": 1.41,
    "Al": 1.21, "Si": 1.11, "P": 1.07, "S": 1.05, "Cl": 1.02, "Ar": 1.06,
    "K": 2.03, "Ca": 1.76, "Ge": 1.20, "As": 1.19, "Se": 1.20, "Br": 1.20,
    "Kr": 1.16, "Sb": 1.39, "Te": 1.38, "I": 1.39, "Xe": 1.40,
}
# Elements whose contacts are coordination, not bonds.
NONMETAL = {"H", "He", "B", "C", "N", "O", "F", "Ne", "Si", "P", "S", "Cl",
            "Ar", "Ge", "As", "Se", "Br", "Kr", "Sb", "Te", "I", "Xe"}
BOND_TOL = 1.2
MAX_MAPS = 5000       # equivalent maps tried per copy


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
    """-> (indices in A, indices in B), pairwise, for a typed map."""
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


def bond_graph(els, xyz):
    """Adjacency sets from coordinates; bonds to metal atoms are left out."""
    n = len(els)
    adj = [set() for _ in range(n)]
    for i in range(n):
        if els[i] not in NONMETAL:
            continue
        for j in range(i + 1, n):
            if els[j] not in NONMETAL:
                continue
            d = math.dist(xyz[i], xyz[j])
            if 0.4 < d < BOND_TOL * (RADIUS.get(els[i], 1.5) +
                                     RADIUS.get(els[j], 1.5)):
                adj[i].add(j)
                adj[j].add(i)
    return adj


def components(adj):
    seen, out = set(), []
    for s0 in range(len(adj)):
        if s0 in seen:
            continue
        comp, todo = [], [s0]
        seen.add(s0)
        while todo:
            u = todo.pop()
            comp.append(u)
            for v in adj[u]:
                if v not in seen:
                    seen.add(v)
                    todo.append(v)
        out.append(sorted(comp))
    return out


def formula(els):
    cnt = {}
    for e in els:
        cnt[e] = cnt.get(e, 0) + 1
    return " ".join(e + (str(c) if c > 1 else "") for e, c in cnt.items())


def _isomorphisms(sels, sadj, bels, badj, comp, limit):
    """Element-labelled isomorphisms of the small graph onto the component
    comp of the big one: lists m with m[s] = big atom.  -> (maps, capped)."""
    n = len(sels)
    start = min(range(n), key=lambda i: (-len(sadj[i]), sels[i]))
    order, parent, seen = [start], {start: None}, {start}
    for u in order:
        for v in sorted(sadj[u]):
            if v not in seen:
                seen.add(v)
                parent[v] = u
                order.append(v)
    maps, m, used = [], {}, set()
    state = {"capped": False}

    def rec(k):
        if state["capped"]:
            return
        if k == n:
            if len(maps) >= limit:
                state["capped"] = True
                return
            maps.append([m[i] for i in range(n)])
            return
        s = order[k]
        cands = comp if parent[s] is None else sorted(badj[m[parent[s]]])
        for c in cands:
            if (c in used or bels[c] != sels[s]
                    or len(badj[c]) != len(sadj[s])):
                continue
            if any(t in m and m[t] not in badj[c] for t in sadj[s]):
                continue
            m[s] = c
            used.add(c)
            rec(k + 1)
            del m[s]
            used.discard(c)

    rec(0)
    return maps, state["capped"]


def _occurrence(ia, ib, xa, xb):
    rmsd, aligned, _rot = D.kabsch_rotation([xa[i] for i in ia],
                                            [xb[i] for i in ib])
    dist = [math.dist(aligned[k], xa[ia[k]]) for k in range(len(ia))]
    return {"ia": list(ia), "ib": list(ib), "rmsd": rmsd, "dist": dist}


def auto_map(ela, xa, elb, xb, name_a="A", name_b="B"):
    """The smaller structure as a bonded unit in the larger one.
    -> (occurrences, capped, free) with free 'a' or 'b': the smaller side."""
    na, nb = len(ela), len(elb)
    if na == nb and ela == elb:
        return [_occurrence(range(na), range(nb), xa, xb)], False, "a"
    free = "a" if na <= nb else "b"
    sels, sx, sname = (ela, xa, name_a) if free == "a" else (elb, xb, name_b)
    bels, bx, bname = (elb, xb, name_b) if free == "a" else (ela, xa, name_a)
    if len(sels) < 3:
        raise ValueError("%s has fewer than 3 atoms: nothing to fit"
                         % sname)
    sadj = bond_graph(sels, sx)
    if len(components(sadj)) != 1:
        raise ValueError("%s (%s) is not one bonded unit (bonds to metal "
                         "atoms do not count); give --atoms"
                         % (sname, formula(sels)))
    badj = bond_graph(bels, bx)
    occ, capped = [], False
    for comp in components(badj):
        if len(comp) != len(sels) or \
                sorted(bels[c] for c in comp) != sorted(sels):
            continue
        maps, cp = _isomorphisms(sels, sadj, bels, badj, comp, MAX_MAPS)
        capped = capped or cp
        best = None
        for mp in maps:
            ia, ib = (list(range(len(sels))), mp) if free == "a" else \
                (mp, list(range(len(sels))))
            o = _occurrence(ia, ib, xa, xb)
            if best is None or o["rmsd"] < best["rmsd"]:
                best = o
        if best:
            occ.append(best)
    if not occ:
        raise ValueError("%s (%s) not found in %s as a bonded unit (same "
                         "elements and bonds; bonds to metal atoms do not "
                         "count)" % (sname, formula(sels), bname))
    occ.sort(key=lambda o: min(o["ia"] if free == "b" else o["ib"]))
    return occ, capped, free


def label(els, idx):
    return " ".join("%s%d" % (els[i], i + 1) for i in idx)


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


def _raw(mode, idx, rot, masses):
    """The mode's motion of atoms idx, rotated and mass-weighted."""
    v = []
    for a in idx:
        d = mode[a]
        if rot:
            d = [sum(rot[i][k] * d[k] for k in range(3)) for i in range(3)]
        w = math.sqrt(masses[a])
        v.extend(c * w for c in d)
    return v


def _unit(v):
    nrm = math.sqrt(sum(c * c for c in v))
    return None if nrm < 1e-10 else [c / nrm for c in v]


def bandshift(a, b, atoms=None, threshold=0.5, names=("A", "B")):
    """a, b: read_modes() results.  -> dict, see the module docstring."""
    ela, xa, fa, ira, ma = a
    elb, xb, fb, irb, mb = b
    capped, auto = False, False
    free = "a"
    if atoms and atoms != "auto":
        ia, ib = common_atoms(atoms, len(xa), len(xb))
        for i, j in zip(ia, ib):
            if ela[i] != elb[j]:
                raise ValueError("atom %d of A is %s but atom %d of B is %s: "
                                 "mapped atoms must be the same element"
                                 % (i + 1, ela[i], j + 1, elb[j]))
        occ = [_occurrence(ia, ib, xa, xb)]
    else:
        auto = True
        occ, capped, free = auto_map(ela, xa, elb, xb, names[0], names[1])
    massa = [MASS.get(e) for e in ela]
    massb = [MASS.get(e) for e in elb]
    for o in occ:
        if any(massa[i] is None for i in o["ia"]) or \
                any(massb[i] is None for i in o["ib"]):
            raise ValueError("an element of the common atoms has no known mass")

    # F: the free (repeated) side, B: the side that holds the copies; the
    # copies' modes are rotated into F's frame.
    if free == "a":
        fm, fmass, bm, bmass, fk, bk = ma, massa, mb, massb, "ia", "ib"
        fx, bx = xa, xb
    else:
        fm, fmass, bm, bmass, fk, bk = mb, massb, ma, massa, "ib", "ia"
        fx, bx = xb, xa
    vf = [_unit(_raw(m, occ[0][fk], None, fmass)) for m in fm]
    rbig = []                       # rbig[j][o]
    rots = []
    for o in occ:
        _r, _al, rot = D.kabsch_rotation([fx[i] for i in o[fk]],
                                         [bx[i] for i in o[bk]])
        rots.append(rot)
    # Normalised over ALL atoms of the copy side: a mode of the complex that
    # hardly moves the common atoms (a C-H stretch of a ligand) must not look
    # like a free-molecule band just because its small motion there has the
    # right shape.
    nrm = []
    allb = list(range(len(bx)))
    for m in bm:
        per = [_raw(m, o[bk], rots[k], bmass) for k, o in enumerate(occ)]
        rbig.append(per)
        on = math.sqrt(sum(c * c for v in per for c in v))
        full = math.sqrt(sum(c * c for c in _raw(m, allb, None, bmass)))
        nrm.append(None if on < 1e-10 else full)

    def overlap(x, j):
        return math.sqrt(sum(sum(p * q for p, q in zip(x, v)) ** 2
                             for v in rbig[j])) / nrm[j]

    pairs = []                      # (free mode, copy-side mode, overlap)
    if len(occ) == 1:
        cand = sorted((-overlap(x, j), i, j)
                      for i, x in enumerate(vf) if x is not None
                      for j in range(len(bm)) if nrm[j] is not None
                      if overlap(x, j) >= threshold)
        used_f, used_b = set(), set()
        for no, i, j in cand:
            if i in used_f or j in used_b:
                continue
            used_f.add(i)
            used_b.add(j)
            pairs.append((i, j, -no))
    else:
        for j in range(len(bm)):
            if nrm[j] is None:
                continue
            best = max(((overlap(x, j), i) for i, x in enumerate(vf)
                        if x is not None), default=None)
            if best and best[0] >= threshold:
                pairs.append((best[1], j, best[0]))
    partners = {}
    for i, _j, _o in pairs:
        partners[i] = partners.get(i, 0) + 1
    if free == "a":
        pairs = [(i, j, o, partners[i]) for i, j, o in pairs]
    else:
        pairs = [(j, i, o, partners[i]) for i, j, o in pairs]
    pairs.sort(key=lambda p: (fa[p[0]], fb[p[1]]))
    used_a = {p[0] for p in pairs}
    used_b = {p[1] for p in pairs}

    def band(freqs, ir, i):
        return {"mode": i + 1, "freq": freqs[i],
                "ir": ir[i] if ir else None}

    matches = []
    for i, j, o, sp in pairs:
        ba, bb = band(fa, ira, i), band(fb, irb, j)
        matches.append({"mode_a": ba["mode"], "freq_a": ba["freq"],
                        "ir_a": ba["ir"], "mode_b": bb["mode"],
                        "freq_b": bb["freq"], "ir_b": bb["ir"],
                        "shift": bb["freq"] - ba["freq"], "overlap": o,
                        "split": sp})

    def rest(freqs, ir, used, side):
        out = []
        for i in range(len(freqs)):
            if i in used:
                continue
            if side == free:
                dead = vf[i] is None
            else:
                dead = nrm[i] is None
            r = band(freqs, ir, i)
            r["reason"] = ("no motion of the common atoms" if dead else
                           "overlap below %g" % threshold)
            out.append(r)
        return out

    occs = []
    for o in occ:
        occs.append({"atoms_a": [i + 1 for i in o["ia"]],
                     "atoms_b": [i + 1 for i in o["ib"]],
                     "label_a": label(ela, o["ia"]),
                     "label_b": label(elb, o["ib"]),
                     "rmsd": o["rmsd"], "dist": o["dist"]})
    return {"n_common": len(occ[0]["ia"]),
            "fit_rmsd": max(o["rmsd"] for o in occ), "threshold": threshold,
            "auto": auto, "capped": capped, "free": free,
            "occurrences": occs, "matches": matches,
            "unmatched_a": rest(fa, ira, used_a, "a"),
            "unmatched_b": rest(fb, irb, used_b, "b")}


COLS = ["mode_a", "freq_a", "ir_a", "mode_b", "freq_b", "ir_b", "shift",
        "overlap", "split"]


def _num(v, fmt):
    return "-" if v is None else fmt % v


def common_line(o):
    return "common atoms: %s = %s, fit RMSD %.2f A (largest atom distance " \
           "%.2f A)" % (o["label_a"], o["label_b"], o["rmsd"], max(o["dist"]))


def as_text(res, na, nb):
    out = ["A: %s   B: %s   overlap threshold %g" % (na, nb, res["threshold"])]
    out.extend(common_line(o) for o in res["occurrences"])
    if res["capped"]:
        out.append("note: more equivalent atom maps than the %d tried per "
                   "copy; the best of those tried is used" % MAX_MAPS)
    out.append("%6s %9s %9s  %6s %9s %9s  %8s %7s" %
               ("mode A", "freq A", "IR A", "mode B", "freq B", "IR B",
                "shift", "overlap"))
    for m in res["matches"]:
        out.append("%6d %9.1f %9s  %6d %9.1f %9s  %+8.1f %7.3f" % (
            m["mode_a"], m["freq_a"], _num(m["ir_a"], "%.1f"), m["mode_b"],
            m["freq_b"], _num(m["ir_b"], "%.1f"), m["shift"], m["overlap"])
            + ("   split over %d bands" % m["split"] if m["split"] > 1 else ""))
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
                    help="override the automatic map: '1-12' or '1-12:5-16' "
                         "(A:B)")
    ap.add_argument("--name-a", default="")
    ap.add_argument("--name-b", default="")
    ap.add_argument("--threshold", type=float, default=0.5)
    ap.add_argument("--lines", action="store_true",
                    help="tab-separated lines for the GUI")
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--json", action="store_true")
    g.add_argument("--csv", action="store_true")
    o = ap.parse_args(argv)
    try:
        res = bandshift(read_modes(o.calc_a), read_modes(o.calc_b),
                        o.atoms, o.threshold,
                        (o.name_a or os.path.basename(os.path.normpath(o.calc_a)),
                         o.name_b or os.path.basename(os.path.normpath(o.calc_b))))
    except ValueError as e:
        print("ecce-bandshift: %s" % e, file=sys.stderr)
        return 1
    na = os.path.basename(os.path.normpath(o.calc_a))
    nb = os.path.basename(os.path.normpath(o.calc_b))
    if o.lines:
        print("header\t%d\t%.6f\t%g" % (res["n_common"], res["fit_rmsd"],
                                          res["threshold"]))
        for oc in res["occurrences"]:
            print("common\t%s\t%s\t%.4f\t%.4f" % (
                oc["label_a"], oc["label_b"], oc["rmsd"], max(oc["dist"])))
        if res["capped"]:
            print("note\tmore equivalent atom maps than the %d tried per "
                  "copy" % MAX_MAPS)
        for m in res["matches"]:
            print("match\t%d\t%.4f\t%s\t%d\t%.4f\t%s\t%.4f\t%.6f\t%d" % (
                m["mode_a"], m["freq_a"], _num(m["ir_a"], "%.4f"),
                m["mode_b"], m["freq_b"], _num(m["ir_b"], "%.4f"),
                m["shift"], m["overlap"], m["split"]))
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
