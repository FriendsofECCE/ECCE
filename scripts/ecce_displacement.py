#!/usr/bin/env python3
"""How far a structure moved (#701): per-element displacement and aligned RMSD.

Wx-free: the Viewer panel and the command line both display this.  Reads the
stored calculation the way the other ecce- tools do: a calculation folder with
Props/GEOMTRACE (a tsvectable, one <step> per geometry frame, rows atoms x 3
columns X Y Z, Angstrom) and Parameters/chemsys.mvm (the atom_list gives the
element of every atom when GEOMTRACE's rowLabels are bare indices).

Usage: ecce_displacement.py <calc> [<calc2>] [--select ...] [--json|--csv]

One calculation: per-element max and rms displacement from the start of the
run's geometry trace (frames share a coordinate frame, so no alignment).

Two calculations: Kabsch (proper rotation, no reflection) aligns the final
geometry of <calc2> onto the final geometry of <calc>, then reports the
aligned RMSD and the per-element displacement after the fit, over all atoms
or the --select selection.  Atoms must be the same, in the same order.

--select SPEC: 1-based atom indices (comma/space separated) or
"within R of i" -- every atom within R Angstrom of atom i in the reference
structure (the pocket around an ion).
"""
import argparse
import csv
import glob
import io
import json
import math
import os
import re
import sys
import xml.etree.ElementTree as ET


def _dist(a, b):
    return math.sqrt(sum((a[k] - b[k]) ** 2 for k in range(3)))


# --- stored-calculation reading ------------------------------------------

def _elements_from_rowlabels(tokens, n):
    """rowLabels like "0 1 2" (indices only) or "1-Co 2-Cl 3-O": the element
    part when present, else None per atom."""
    els = []
    for t in tokens[:n]:
        m = re.match(r"^(\d+)?[-_ ]?(.*)$", t)
        rest = m.group(2).strip()
        els.append(rest if rest else None)
    return els


def _elements_from_chemsys(calc, n):
    path = os.path.join(calc, "Parameters", "chemsys.mvm")
    try:
        lines = open(path, errors="replace").read().splitlines()
    except OSError:
        return [None] * n
    els = []
    for i, ln in enumerate(lines):
        if re.match(r"^\s*atom_list\s*:", ln):
            for ln2 in lines[i + 1:]:
                if re.match(r"^\s*[A-Za-z_][\w.]*\s*:", ln2):
                    break  # the next key, e.g. "attr_list:"
                f = ln2.split()
                if f:
                    els.append(f[0])
            break
    return (els + [None] * n)[:n]


def read_geomtrace(calc):
    """-> (elements, frames): elements[i] the symbol of atom i (or None),
    frames a list of geometries, each a list of n (x, y, z) triples."""
    path = os.path.join(calc, "Props", "GEOMTRACE")
    if not os.path.isfile(path):
        alts = [p for p in glob.glob(os.path.join(calc, "Props", "GEOMTRACE*"))
                if "/.DAV/" not in p]
        if not alts:
            raise ValueError("%s has no Props/GEOMTRACE" % calc)
        path = alts[0]
    root = ET.parse(path).getroot()
    rows = int(root.get("rows", "0"))
    cols = int(root.get("columns", "3"))
    elements = _elements_from_rowlabels((root.get("rowLabels") or "").split(),
                                        rows)
    blocks = [c for c in root if c.tag in ("step", "matrix")]
    if not blocks and root.text and root.text.strip():
        blocks = [root]
    frames = []
    for b in blocks:
        vals = [float(x) for x in (b.text or "").split()]
        if len(vals) >= rows * cols:
            frames.append([tuple(vals[i * cols:(i + 1) * cols])
                           for i in range(rows)])
    if not frames:
        raise ValueError("%s has an empty GEOMTRACE" % calc)
    if any(e is None for e in elements):
        els = _elements_from_chemsys(calc, rows)
        elements = [elements[i] if elements[i] else els[i]
                    for i in range(rows)]
    return elements, frames


def final_geometry(calc):
    """The last frame of a calculation's geometry trace (a single point's
    one and only frame)."""
    elements, frames = read_geomtrace(calc)
    return elements, frames[-1]


# --- geometry ------------------------------------------------------------

def centroid(pts):
    n = len(pts)
    return [sum(p[k] for p in pts) / n for k in range(3)]


def _dom_eigvec(n4):
    """Eigenvector of the largest *algebraic* eigenvalue of Horn's symmetric
    4x4 matrix -- the optimal rotation quaternion.  Power iteration finds the
    largest magnitude, but for mirror-related structures (det S < 0) the most
    negative eigenvalue has the larger magnitude, so shift by the Frobenius
    norm (>= -(lambda_max + lambda_min) / 2) first; then the largest algebraic
    eigenvalue is also the largest magnitude."""
    shift = math.sqrt(sum(x * x for row in n4 for x in row))
    m = [row[:] for row in n4]
    for i in range(4):
        m[i][i] += shift
    v = [1.0, 2.0, 3.0, 4.0]
    norm = math.sqrt(sum(x * x for x in v))
    v = [x / norm for x in v]
    for _ in range(500):
        w = [sum(m[i][j] * v[j] for j in range(4)) for i in range(4)]
        norm = math.sqrt(sum(x * x for x in w))
        if norm < 1e-30:
            break
        w = [x / norm for x in w]
        if sum((w[i] - v[i]) ** 2 for i in range(4)) < 1e-30:
            v = w
            break
        v = w
    return v


def _rotation_from_quat(q):
    q0, q1, q2, q3 = q
    return [
        [q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3, 2 * (q1 * q2 - q0 * q3),
         2 * (q1 * q3 + q0 * q2)],
        [2 * (q1 * q2 + q0 * q3), q0 * q0 - q1 * q1 + q2 * q2 - q3 * q3,
         2 * (q2 * q3 - q0 * q1)],
        [2 * (q1 * q3 - q0 * q2), 2 * (q2 * q3 + q0 * q1),
         q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3],
    ]


def kabsch(ref, mov):
    """Proper-rotation fit of mov onto ref (Horn's quaternion method, so a
    reflection is never reported as a fit).  Returns (rmsd, aligned) with
    aligned in ref's frame (mov translated onto ref's centroid)."""
    ref = [tuple(map(float, p)) for p in ref]
    mov = [tuple(map(float, p)) for p in mov]
    n = len(ref)
    if n != len(mov) or n == 0:
        raise ValueError("the two structures must have the same number of atoms")
    cr, cm = centroid(ref), centroid(mov)
    p = [[ref[i][k] - cr[k] for k in range(3)] for i in range(n)]
    q = [[mov[i][k] - cm[k] for k in range(3)] for i in range(n)]
    # Cross-covariance S[i][j] = sum_k q[k][i] * p[k][j].
    s = [[sum(q[k][i] * p[k][j] for k in range(n)) for j in range(3)]
         for i in range(3)]
    sxx, sxy, sxz = s[0]
    syx, syy, syz = s[1]
    szx, szy, szz = s[2]
    n4 = [
        [sxx + syy + szz, syz - szy, szx - sxz, sxy - syx],
        [syz - szy, sxx - syy - szz, sxy + syx, szx + sxz],
        [szx - sxz, sxy + syx, -sxx + syy - szz, syz + szy],
        [sxy - syx, szx + sxz, syz + szy, -sxx - syy + szz],
    ]
    r = _rotation_from_quat(_dom_eigvec(n4))
    aligned = [[sum(r[i][k] * q[j][k] for k in range(3)) + cr[i]
                for i in range(3)] for j in range(n)]
    rmsd = math.sqrt(sum(sum((aligned[j][i] - ref[j][i]) ** 2 for i in range(3))
                         for j in range(n)) / n)
    return rmsd, aligned


def per_element(a, b):
    """Displacement b - a per atom: dx, dy, dz and the length, in order."""
    return [{"dx": b[i][0] - a[i][0], "dy": b[i][1] - a[i][1],
             "dz": b[i][2] - a[i][2],
             "displacement": _dist(a[i], b[i])} for i in range(len(a))]


def trace_stats(frames):
    """Per-atom max and rms displacement from the first frame over a run."""
    start = frames[0]
    n = len(start)
    sumsq = [0.0] * n
    maxima = [0.0] * n
    for f in frames[1:]:
        for i in range(n):
            d2 = sum((f[i][k] - start[i][k]) ** 2 for k in range(3))
            sumsq[i] += d2
            d = math.sqrt(d2)
            if d > maxima[i]:
                maxima[i] = d
    steps = max(1, len(frames) - 1)
    return [{"max": maxima[i], "rms": math.sqrt(sumsq[i] / steps)}
            for i in range(n)]


def select_indices(spec, ref):
    """Parse --select against the reference coordinates (list of triples).
    Returns a sorted list of 0-based indices, or None for all atoms."""
    if not spec:
        return None
    m = re.match(r"^\s*within\s+([0-9]*\.?[0-9]+)\s*(?:A|Å|angstrom)?\s+of\s+"
                 r"(?:atom\s+)?(\d+)\s*$", spec, re.I)
    if m:
        r = float(m.group(1))
        i = int(m.group(2)) - 1
        if not 0 <= i < len(ref):
            raise ValueError("atom %d is out of range" % (i + 1))
        return sorted(j for j, c in enumerate(ref) if _dist(c, ref[i]) <= r)
    out = []
    for tok in re.split(r"[\s,]+", spec.strip()):
        if not tok:
            continue
        i = int(tok) - 1
        if not 0 <= i < len(ref):
            raise ValueError("atom %d is out of range" % (i + 1))
        out.append(i)
    return sorted(set(out))


# --- output --------------------------------------------------------------

def _fmt_rows(rows, fmt, cols):
    if fmt == "json":
        return json.dumps(rows, indent=1)
    if fmt == "csv":
        s = io.StringIO()
        w = csv.writer(s, lineterminator="\n")
        w.writerow(cols)
        for r in rows:
            w.writerow([r[c] for c in cols])
        return s.getvalue().rstrip("\n")
    lines = ["\t".join(cols)]
    for r in rows:
        lines.append("\t".join("" if r[c] is None else str(r[c]) for c in cols))
    return "\n".join(lines)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("calc", help="a stored calculation folder")
    ap.add_argument("calc2", nargs="?", help="second calculation to compare")
    ap.add_argument("--select", metavar="SPEC",
                    help="1-based atom indices, or 'within R of i'")
    fmt = ap.add_mutually_exclusive_group()
    fmt.add_argument("--json", action="store_true", help="JSON output")
    fmt.add_argument("--csv", action="store_true", help="CSV output")
    a = ap.parse_args(argv)
    out = "json" if a.json else "csv" if a.csv else "text"

    if a.calc2:
        el1, g1 = final_geometry(a.calc)
        el2, g2 = final_geometry(a.calc2)
        if len(g1) != len(g2):
            sys.exit("the two calculations have different numbers of atoms "
                     "(%d vs %d)" % (len(g1), len(g2)))
        sel = select_indices(a.select, g1)
        ref = g1 if sel is None else [g1[i] for i in sel]
        mov = g2 if sel is None else [g2[i] for i in sel]
        rmsd, aligned = kabsch(ref, mov)
        disp = per_element(ref, aligned)
        atoms = [{"index": (sel[i] if sel else i) + 1,
                  "element": el1[sel[i] if sel else i] or "",
                  "dx": round(disp[i]["dx"], 6),
                  "dy": round(disp[i]["dy"], 6),
                  "dz": round(disp[i]["dz"], 6),
                  "displacement": round(disp[i]["displacement"], 6)}
                 for i in range(len(ref))]
        if out == "json":
            print(json.dumps({"rmsd": round(rmsd, 6), "n_atoms": len(ref),
                              "selection": [i + 1 for i in sel] if sel else None,
                              "atoms": atoms}, indent=1))
        else:
            print("aligned RMSD: %.6f Angstrom (%d atoms%s)"
                  % (rmsd, len(ref), "" if sel is None else ", selection"))
            print(_fmt_rows(atoms, out, ["index", "element", "dx", "dy", "dz",
                                         "displacement"]))
        return 0

    elements, frames = read_geomtrace(a.calc)
    sel = select_indices(a.select, frames[0])
    stats = trace_stats(frames)
    idx = sel if sel is not None else range(len(frames[0]))
    atoms = [{"index": i + 1, "element": elements[i] or "",
              "max": round(stats[i]["max"], 6),
              "rms": round(stats[i]["rms"], 6)} for i in idx]
    if out == "json":
        print(json.dumps({"n_atoms": len(idx), "frames": len(frames),
                          "selection": [i + 1 for i in sel] if sel else None,
                          "atoms": atoms}, indent=1))
    else:
        print("geometry trace: %d frames, displacement from the start "
              "(%d atoms%s)" % (len(frames), len(idx),
                                "" if sel is None else ", selection"))
        print(_fmt_rows(atoms, out, ["index", "element", "max", "rms"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
