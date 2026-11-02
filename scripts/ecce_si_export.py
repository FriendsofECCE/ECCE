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
       [--include-excluded] [--table-file FILE]      (default: the .xyz name with .csv / .tex)
       ecce_si_export.py --checks csv|latex <calc>... [-o FILE]   (#615)

--checks writes the Checks table instead: one row per calculation with the
state, termination, optimisation and SCF convergence (from the stored
ecce:progress snapshot), imaginary modes, <S^2> and its deviation from
S(S+1), charge, multiplicity, method, basis, solvent, E, ZPE, G, the
exclusion reason, and a "problems" column naming what looks wrong.  A value
that is not stored is "-", never 0.
"""
import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

MULT = {"singlet": 1, "doublet": 2, "triplet": 3, "quartet": 4, "quintet": 5,
        "sextet": 6, "septet": 7, "octet": 8, "nonet": 9, "decet": 10}
FIELDS = ["name", "code", "method", "charge", "mult", "E", "G", "nimag",
          "excluded"]


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
        "nimag": nimag, "excluded": _meta(meta, "excluded"),
        "elements": els, "coords": xyz,
    }


CHECK_FIELDS = ["name", "state", "normal_termination", "opt_converged",
                "scf_converged", "imaginary_modes", "S2", "S2_deviation",
                "charge", "multiplicity", "method", "basis", "solvent",
                "E_el", "ZPE", "G", "excluded", "problems"]
S2_TOL = 0.1


def _progress_stages(text):
    """ecce:progress snapshot -> {stage: {key: value}}, the last line of a stage wins."""
    st = {}
    for ln in re.split(r"\\n|\n", text):
        w = ln.split()
        if not w or w[0].startswith("#") or w[0].endswith("~"):
            continue
        st[w[0]] = dict(t.split("=", 1) for t in w[1:] if "=" in t)
    return st


def _converged(stage, complete, plain_ok=True):
    """yes/no/- for one stage; a Complete run whose last opt step meets every
    criterion counts as converged even without the code's own line (as the
    Progress box does)."""
    if stage is None:
        return "-"
    c = stage.get("converged")
    if c in ("yes", "no"):
        return c
    if complete:
        met = tot = 0
        for k, v in stage.items():
            m = re.match(r"^(-?[\d.eE+-]+)/([\d.eE+-]+)$", v)
            if k == "step" or not m:
                continue
            tot += 1
            met += abs(float(m.group(1))) <= float(m.group(2))
        if tot and met == tot:
            return "yes"
    return "-"


def read_checks(calc):
    calc = calc.rstrip("/")
    meta = os.path.join(calc, ".ecce-meta")
    state = _meta(meta, "state") or "-"
    low = state.lower()
    complete = low in ("complete", "completed", "loaded")
    normal = "yes" if complete else ("no" if re.search(
        r"fail|kill|unsucc", low) else "-")
    stages = _progress_stages(_meta(meta, "progress"))
    opt = _converged(stages.get("opt"), complete)
    scf = _converged(stages.get("scf"), complete)
    prop = lambda n: _value_text(os.path.join(calc, "Props", n)) or "-"
    nimag = "-"
    t = _value_text(os.path.join(calc, "Props", "VIBFREQ"))
    if t is not None:
        nimag = str(sum(1 for v in t.split() if float(v) < 0))
    elif "imaginary" in stages.get("freq", {}):
        nimag = stages["freq"]["imaginary"]
    mult = MULT.get(_meta(meta, "spinmultiplicity").lower())
    try:
        charge = str(_chemsys(calc)[2])
    except (OSError, ValueError):
        charge = "-"
    s2, dev = prop("S2"), "-"
    if s2 != "-" and mult:
        S = (mult - 1) / 2.0
        dev = "%.4f" % (float(s2) - S * (S + 1))
    runtype = _meta(meta, "runtype")
    ts = re.search(r"transition|saddle", runtype, re.I) is not None
    minimum = re.search(r"geometry|geovib|vibration|frequenc|optimi", runtype,
                        re.I) is not None
    prob = []
    if normal == "no":
        prob.append("abnormal termination")
    if opt == "no" or scf == "no":
        prob.append("not converged")
    if nimag != "-" and (ts or minimum) and int(nimag) != (1 if ts else 0):
        prob.append("%s imaginary mode(s), expected %d" % (nimag, 1 if ts else 0))
    if dev != "-" and abs(float(dev)) > S2_TOL:
        prob.append("<S2> deviates %s" % dev)
    basis = _meta(os.path.join(calc, "Parameters", ".ecce-meta"), "name",
                  "BasisSet.ecce_basisset")
    return {
        "name": re.sub(r"\s+", "_", os.path.basename(calc)), "state": state,
        "normal_termination": normal, "opt_converged": opt, "scf_converged": scf,
        "imaginary_modes": nimag, "S2": s2, "S2_deviation": dev,
        "charge": charge, "multiplicity": mult if mult else "-",
        "method": _meta(meta, "theory") or "-", "basis": basis or "-",
        "solvent": _value_text(os.path.join(calc, "Props", "SOLVMODEL")) or "-",
        "E_el": prop("TE"), "ZPE": prop("EZEROPT"), "G": prop("EGIBBS"),
        "excluded": _meta(meta, "excluded"), "problems": "; ".join(prob),
    }


def checks_latex(recs):
    head = ["Name", "State", "Normal", "Opt.", "SCF", "Imag.", "$\\langle S^2\\rangle$",
            "Dev.", "Charge", "Mult.", "Method", "Basis", "Solvent",
            "$E$ (E$_\\mathrm{h}$)", "ZPE (E$_\\mathrm{h}$)", "$G$ (E$_\\mathrm{h}$)",
            "Excluded", "Problems"]
    rows = [" & ".join(_tex(r[k]) for k in CHECK_FIELDS) + r" \\" for r in recs]
    return "\n".join([r"\begin{tabular}{" + "l" * 2 + "c" * 4 + "r" * 4 +
                      "lll" + "r" * 3 + "ll}", r"\hline",
                      " & ".join(head) + r" \\", r"\hline"] + rows +
                     [r"\hline", r"\end{tabular}"]) + "\n"


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


def csv_text(recs, fields=None):
    fields = fields or FIELDS

    def q(v):
        v = str(v)
        return '"%s"' % v.replace('"', '""') if re.search(r'[",\n]', v) else v
    return "\n".join([",".join(fields)] +
                     [",".join(q(r[k]) for k in fields) for r in recs]) + "\n"


def latex_text(recs):
    head = ["Name", "Code", "Method", "Charge", "Mult.", "$E$ (E$_\\mathrm{h}$)",
            "$G$ (E$_\\mathrm{h}$)", "Imag. freq.", "Excluded"]
    rows = [" & ".join(_tex(r[k]) for k in FIELDS) + r" \\" for r in recs]
    return "\n".join([r"\begin{tabular}{lllrrrrrl}", r"\hline",
                      " & ".join(head) + r" \\", r"\hline"] + rows +
                     [r"\hline", r"\end{tabular}"]) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("calcs", nargs="+")
    ap.add_argument("-o", "--output")
    ap.add_argument("--checks", choices=["csv", "latex"],
                    help="write the Checks table (#615) instead of the xyz file")
    ap.add_argument("--table", choices=["csv", "latex"])
    ap.add_argument("--table-file")
    ap.add_argument("--include-excluded", action="store_true",
                    help="also list calculations marked excluded (ecce:excluded)")
    a = ap.parse_args(argv)
    if a.checks:
        try:
            recs = [read_checks(c) for c in a.calcs]
        except (OSError, ValueError) as e:
            print("ecce-si-export: %s" % e, file=sys.stderr)
            return 1
        text = csv_text(recs, CHECK_FIELDS) if a.checks == "csv" \
            else checks_latex(recs)
        if a.output:
            open(a.output, "w").write(text)
        else:
            sys.stdout.write(text)
        return 0
    if not a.output:
        ap.error("-o is required without --checks")
    recs = []
    for c in a.calcs:
        try:
            recs.append(read_calc(c))
        except (OSError, ValueError) as e:
            print("ecce-si-export: %s: %s" % (c, e), file=sys.stderr)
            return 1
    if not a.include_excluded:
        recs = [r for r in recs if not r["excluded"]]
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
