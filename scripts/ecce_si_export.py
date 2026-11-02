#!/usr/bin/env python3
"""Coordinates and energies of stored calculations for Supporting Information (#625).

Wx-free; the Organizer's File > Export > Supporting Information... runs it.
Reads stored calculation folders (Props/TE, EGIBBS, VIBFREQ, GEOMTRACE,
Parameters/chemsys.mvm, .ecce-meta) and writes one multi-structure .xyz: per
calculation the atom count, a comment line

  name  charge=Q  mult=M  E=<Eh>  G=<Eh or ->  code=<code> method=<theory/basis>

and the final geometry in Angstrom.  --table csv|latex adds a table of the
same fields plus the number of imaginary frequencies.  This is a listing: the
numbers of different codes and methods are never combined.

Usage: ecce_si_export.py <calc>... -o out.xyz [--table csv|latex]
       [--table-file FILE]      (default: the .xyz name with .csv / .tex)
"""
import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

MULT = {"singlet": 1, "doublet": 2, "triplet": 3, "quartet": 4, "quintet": 5,
        "sextet": 6, "septet": 7, "octet": 8, "nonet": 9, "decet": 10}
FIELDS = ["name", "code", "method", "charge", "mult", "E", "G", "nimag"]


def _meta(path, key, owner="."):
    """Value of ecce:<key> for `owner` in a .ecce-meta file (tab separated)."""
    try:
        for ln in open(path, errors="replace"):
            f = ln.rstrip("\n").split("\t")   # owner, key, "", value
            if len(f) >= 3 and f[0] == owner and f[1].endswith(":" + key):
                return f[-1].strip()
    except OSError:
        pass
    return ""


def _value_text(path):
    """The stored number exactly as written, or None."""
    try:
        t = ET.parse(path).getroot().text
    except (OSError, ET.ParseError):
        return None
    return t.strip() if t and t.strip() else None


def _chemsys(calc):
    """-> (elements, coords, charge) from Parameters/chemsys.mvm."""
    els, xyz, charge = [], [], 0
    lines = open(os.path.join(calc, "Parameters", "chemsys.mvm"),
                 errors="replace").read().splitlines()
    inlist = False
    for ln in lines:
        m = re.match(r"^\s*charge\s*:\s*(-?\d+)", ln)
        if m:
            charge = int(m.group(1))
        if re.match(r"^\s*atom_list\s*:", ln):
            inlist = True
        elif inlist:
            if re.match(r"^\s*[A-Za-z_][\w.]*\s*:", ln):
                inlist = False
                continue
            f = ln.split()
            if len(f) >= 4:
                els.append(f[0])
                xyz.append([float(v) for v in f[1:4]])
    return els, xyz, charge


def _final_frame(calc, n):
    """The last GEOMTRACE frame as n x 3 (Angstrom), or None."""
    path = os.path.join(calc, "Props", "GEOMTRACE")
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError):
        return None
    rows, cols = int(root.get("rows", "0")), int(root.get("columns", "3"))
    steps = [c for c in root if c.tag in ("step", "matrix")] or [root]
    if rows != n or cols != 3:
        return None
    vals = [float(x) for x in (steps[-1].text or "").split()]
    if len(vals) < 3 * n:
        return None
    return [vals[3 * i:3 * i + 3] for i in range(n)]


def read_calc(calc):
    calc = calc.rstrip("/")
    els, xyz, charge = _chemsys(calc)
    frame = _final_frame(calc, len(els))
    if frame:
        xyz = frame
    meta = os.path.join(calc, ".ecce-meta")
    mult = MULT.get(_meta(meta, "spinmultiplicity").lower(), 1)
    basis = _meta(os.path.join(calc, "Parameters", ".ecce-meta"), "name",
                  "BasisSet.ecce_basisset")
    theory = _meta(meta, "theory")
    method = "/".join(p for p in (theory, basis) if p) or "-"
    nimag = "-"
    vf = os.path.join(calc, "Props", "VIBFREQ")
    if os.path.isfile(vf):
        t = _value_text(vf)
        if t is not None:
            nimag = str(sum(1 for v in t.split() if float(v) < 0))
    return {
        "name": re.sub(r"\s+", "_", os.path.basename(calc)),
        "code": _meta(meta, "application") or "-",
        "method": method, "charge": charge, "mult": mult,
        "E": _value_text(os.path.join(calc, "Props", "TE")) or "-",
        "G": _value_text(os.path.join(calc, "Props", "EGIBBS")) or "-",
        "nimag": nimag, "elements": els, "coords": xyz,
    }


def xyz_text(recs):
    out = []
    for r in recs:
        out.append("%d" % len(r["elements"]))
        out.append("%s  charge=%d  mult=%d  E=%s  G=%s  code=%s method=%s" % (
            r["name"], r["charge"], r["mult"], r["E"], r["G"],
            r["code"], r["method"]))
        for e, c in zip(r["elements"], r["coords"]):
            out.append("%-2s %16.10f %16.10f %16.10f" % ((e,) + tuple(c)))
    return "\n".join(out) + "\n"


def _tex(s):
    s = str(s).replace("\\", r"\textbackslash{}")
    for ch in "&%$#_{}":
        s = s.replace(ch, "\\" + ch)
    return s.replace("~", r"\textasciitilde{}").replace("^", r"\textasciicircum{}")


def csv_text(recs):
    def q(v):
        v = str(v)
        return '"%s"' % v.replace('"', '""') if re.search(r'[",\n]', v) else v
    return "\n".join([",".join(FIELDS)] +
                     [",".join(q(r[k]) for k in FIELDS) for r in recs]) + "\n"


def latex_text(recs):
    head = ["Name", "Code", "Method", "Charge", "Mult.", "$E$ (E$_\\mathrm{h}$)",
            "$G$ (E$_\\mathrm{h}$)", "Imag. freq."]
    rows = [" & ".join(_tex(r[k]) for k in FIELDS) + r" \\" for r in recs]
    return "\n".join([r"\begin{tabular}{lllrrrrr}", r"\hline",
                      " & ".join(head) + r" \\", r"\hline"] + rows +
                     [r"\hline", r"\end{tabular}"]) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("calcs", nargs="+")
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("--table", choices=["csv", "latex"])
    ap.add_argument("--table-file")
    a = ap.parse_args(argv)
    recs = []
    for c in a.calcs:
        try:
            recs.append(read_calc(c))
        except (OSError, ValueError) as e:
            print("ecce-si-export: %s: %s" % (c, e), file=sys.stderr)
            return 1
    open(a.output, "w").write(xyz_text(recs))
    if a.table:
        tf = a.table_file or (os.path.splitext(a.output)[0] +
                              (".csv" if a.table == "csv" else ".tex"))
        open(tf, "w").write(csv_text(recs) if a.table == "csv"
                            else latex_text(recs))
        print(tf)
    return 0


if __name__ == "__main__":
    sys.exit(main())
