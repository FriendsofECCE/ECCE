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
       ecce_si_export.py --checks csv|latex <calc-or-folder>... [-o FILE] [--total]   (#615, #627)

--checks writes the Checks table instead: one row per calculation with the
state, termination, optimisation and SCF convergence (from the stored
ecce:progress snapshot), imaginary modes, <S^2> and its deviation from
S(S+1), charge, multiplicity, method, basis, solvent, E, ZPE, G, the
booked and used core-hours (cores x requested wall time; cores x the job's
own start to end), the exclusion reason, and a "problems" column naming what looks wrong.  A value
that is not stored is "-", never 0.

--rank csv|latex ranks the calculations of a folder (#703): dE and dG to the
lowest (or --ref NAME) in kJ/mol and eV, sorted, also per formula unit
(--per-fu N|auto) and per substituted site (--per-site N).  dG is the stored G,
or ecce-thermo's with --treatment SPEC.  A calculation that differs in code,
method, basis or solvent from the reference is left out of the dE with the
reason named; a different composition (unless --substitution A>B says which
atom was replaced) or cell is only flagged.  --descriptor "name=EXPR" adds a
column from ecce-contacts (metal-donor shells only): EXPR is `pattern`,
`cn:Ti` (donors around the Ti atoms) or `dist:Ti-O` (shortest Ti-O distance,
Angstrom).
"""
import argparse
import json
import math
import os
import subprocess
import tempfile
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
    booked, used = core_hours(meta)
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
                "E_el", "ZPE", "G", "booked_coreh", "used_coreh", "excluded", "problems"]
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


def _parse_date(text):
    """ecce:startdate / completiondate ("Mon, 02 Nov 2026 04:17:30 GMT") -> epoch."""
    from email.utils import parsedate_tz, mktime_tz
    t = parsedate_tz(text) if text else None
    return mktime_tz(t) if t else None


def core_hours(meta):
    """-> (booked, used) core-hours, each a float or None when not stored.

    Booked is cores x the requested wall time ("D H:M"); used is cores x the
    job's own start to end.  A machine entry does not say whether a queue
    books whole nodes, so cores are taken as set (the same rule as
    include/dsm/LaunchInfoRows.H, which the Launch Info rows use).
    """
    try:
        cores = int(_meta(meta, "launch_totalprocs") or 0)
    except ValueError:
        cores = 0
    booked = used = None
    m = re.match(r"^\s*(\d+)\s+(\d+):(\d+)\s*$", _meta(meta, "launch_maxwall"))
    if cores > 0 and m:
        wall = (int(m[1]) * 24 + int(m[2])) * 3600 + int(m[3]) * 60
        if wall > 0:
            booked = cores * wall / 3600.0
    s, e = _parse_date(_meta(meta, "startdate")), _parse_date(_meta(meta, "completiondate"))
    if cores > 0 and s and e and e >= s:
        used = cores * (e - s) / 3600.0
    return booked, used


def _coreh(v):
    return "-" if v is None else "%.1f" % v


def is_calc(path):
    return os.path.isdir(os.path.join(path, "Parameters")) or \
        bool(_meta(os.path.join(path, ".ecce-meta"), "state"))


def expand_calcs(paths):
    """A folder stands for the calculations below it."""
    out = []
    for p in paths:
        if is_calc(p) or not os.path.isdir(p):
            out.append(p)
            continue
        for d, subs, _ in os.walk(p):
            subs[:] = sorted(x for x in subs if not x.startswith("."))
            for x in subs:
                q = os.path.join(d, x)
                if is_calc(q):
                    out.append(q)
    return out


def total_row(recs):
    """Sum of the core-hour columns; a calculation without a value adds nothing."""
    row = {k: "-" for k in CHECK_FIELDS}
    row["name"] = "total"
    for k in ("booked_coreh", "used_coreh"):
        vals = [float(r[k]) for r in recs if r[k] != "-"]
        row[k] = "%.1f" % sum(vals) if vals else "-"
    return row


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
    booked, used = core_hours(meta)
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
        "booked_coreh": _coreh(booked), "used_coreh": _coreh(used),
        "excluded": _meta(meta, "excluded"), "problems": "; ".join(prob),
    }


def checks_latex(recs):
    head = ["Name", "State", "Normal", "Opt.", "SCF", "Imag.", "$\\langle S^2\\rangle$",
            "Dev.", "Charge", "Mult.", "Method", "Basis", "Solvent",
            "$E$ (E$_\\mathrm{h}$)", "ZPE (E$_\\mathrm{h}$)", "$G$ (E$_\\mathrm{h}$)",
            "Booked (core-h)", "Used (core-h)", "Excluded", "Problems"]
    rows = [" & ".join(_tex(r[k]) for k in CHECK_FIELDS) + r" \\" for r in recs]
    return "\n".join([r"\begin{tabular}{" + "l" * 2 + "c" * 4 + "r" * 4 +
                      "lll" + "r" * 5 + "ll}", r"\hline",
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


KJ_PER_EH = 2625.499638
EV_PER_EH = 27.211386246
RANK_BASE = ["name", "composition", "cell", "code", "method", "solvent",
             "E_el", "G", "dE_kJ", "dE_eV", "dG_kJ", "dG_eV"]


def _lattice(calc):
    """The three lattice vectors of Parameters/chemsys.mvm as a tuple, or None."""
    try:
        lines = open(os.path.join(calc, "Parameters", "chemsys.mvm"),
                     errors="replace").read().splitlines()
    except OSError:
        return None
    for i, ln in enumerate(lines):
        if re.match(r"^\s*lattice_vectors\s*:", ln):
            try:
                v = [tuple(float(x) for x in lines[i + k].split()[:3]) for k in (1, 2, 3)]
            except (IndexError, ValueError):
                return None
            return tuple(v) if all(len(r) == 3 for r in v) else None
    return None


def _counts(els, subst=None):
    c = {}
    for e in els:
        c[e] = c.get(e, 0) + 1
    if subst:
        a, b = subst
        if b in c:
            c[a] = c.get(a, 0) + c.pop(b)
    return c


def _hill(c):
    keys = sorted(c, key=lambda e: (e not in ("C",), e) if "C" in c else e)
    if "C" in c and "H" in c:
        keys = ["C", "H"] + sorted(k for k in c if k not in ("C", "H"))
    return "".join(k + (str(c[k]) if c[k] > 1 else "") for k in keys)


def _same_cell(a, b):
    return a is not None and b is not None and all(
        abs(x - y) < 1e-3 for r, q in zip(a, b) for x, y in zip(r, q))


def _contacts_script():
    here = os.path.dirname(os.path.abspath(__file__))
    for n in ("ecce_contacts.py", "ecce-contacts"):
        if os.path.isfile(os.path.join(here, n)):
            return os.path.join(here, n)
    return None


def descriptor_value(rec, expr):
    """One ecce-contacts descriptor of the final geometry, "-" when not available."""
    script = _contacts_script()
    if not script or not rec["elements"]:
        return "-"
    with tempfile.TemporaryDirectory(prefix="rank-") as d:
        f = os.path.join(d, "s.xyz")
        open(f, "w").write("%d\n%s\n" % (len(rec["elements"]), rec["name"]) + "".join(
            "%s %.8f %.8f %.8f\n" % ((e,) + tuple(c))
            for e, c in zip(rec["elements"], rec["coords"])))
        r = subprocess.run([sys.executable, script, "--format", "json", f],
                           capture_output=True, text=True)
    if r.returncode:
        return "-"
    try:
        js = json.loads(r.stdout)
    except ValueError:
        return "-"
    el = lambda lab: re.sub(r"\d+$", "", lab)
    expr = expr.strip()
    if expr == "pattern":
        return js.get("pattern") or "-"
    m = re.match(r"^cn:(\w+)$", expr)
    if m:
        n = sum(1 for s in js["shells"] if el(s["ion"]) == m[1])
        return str(n) if n else "-"
    m = re.match(r"^dist:(\w+)-(\w+)$", expr)
    if m:
        d = [s["distance"] for s in js["shells"]
             if el(s["ion"]) == m[1] and el(s["donor"]) == m[2]]
        return "%.3f" % min(d) if d else "-"
    return "-"


def thermo_G(calc, treatment):
    """Total G (Eh) from ecce-thermo for a calculation, or None."""
    url = _meta(os.path.join(calc, ".ecce-meta"), "sourceurl") or \
        "file://" + os.path.abspath(calc)
    home = os.environ.get("ECCE_HOME", "")
    exe = os.path.join(home, "bin", "ecce-thermo")
    if not os.path.isfile(exe):
        exe = "ecce-thermo"
    try:
        r = subprocess.run([exe, url, "--treatment", treatment, "--json"],
                           capture_output=True, text=True)
        return float(json.loads(r.stdout)[0]["treatments"][treatment]["G"])
    except (OSError, ValueError, KeyError, IndexError):
        return None


def _num(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


def rank(paths, ref=None, per_fu=None, per_site=None, treatment=None,
         descriptors=(), subst=None, sort="E"):
    """-> (fields, rows, warnings).  Never mixes methods in one delta."""
    recs, warns = [], []
    for c in expand_calcs(paths):
        r = read_calc(c)
        r["solvent"] = _value_text(os.path.join(c, "Props", "SOLVMODEL")) or "-"
        r["lattice"] = _lattice(c)
        r["counts"] = _counts(r["elements"], subst)
        r["key"] = (r["code"], r["method"], r["solvent"])
        r["flags"] = []
        if treatment:
            g = thermo_G(c, treatment)
            if g is None:
                r["flags"].append("no G from ecce-thermo (%s)" % treatment)
            r["G"] = "%.10f" % g if g is not None else "-"
        recs.append(r)
    if not recs:
        return RANK_BASE, [], ["no calculations found"]
    ok = [r for r in recs if not r["excluded"] and _num(r["E"]) is not None]
    for r in recs:
        if r["excluded"]:
            r["flags"].append("excluded: %s" % r["excluded"])
        elif _num(r["E"]) is None:
            r["flags"].append("no energy stored")
    base = None
    if ref:
        base = next((r for r in recs if r["name"] == ref), None)
        if base is None or base not in ok:
            return RANK_BASE, [], ["--ref %s is not a usable calculation" % ref]
    elif ok:
        keys = [r["key"] for r in ok]
        top = max(set(keys), key=lambda k: (keys.count(k), -keys.index(k)))
        base = min((r for r in ok if r["key"] == top), key=lambda r: float(r["E"]))
    usable = []
    for r in ok:
        if r["key"] == base["key"]:
            usable.append(r)
            continue
        what = [n for n, a, b in zip(("code", "method/basis", "solvent"), r["key"],
                                     base["key"]) if a != b]
        r["flags"].append("different %s (%s vs %s), left out of the delta" % (
            ", ".join(what), "/".join(r["key"]), "/".join(base["key"])))
    for r in usable:
        if r["counts"] != base["counts"]:
            r["flags"].append("composition %s differs from %s" % (
                _hill(r["counts"]), _hill(base["counts"])))
        if (r["lattice"] is None) != (base["lattice"] is None) or (
                r["lattice"] is not None and not _same_cell(r["lattice"], base["lattice"])):
            r["flags"].append("cell differs from %s" % base["name"])
    e0 = float(base["E"]) if ref else min(float(r["E"]) for r in usable)
    gs = [float(r["G"]) for r in usable if _num(r["G"]) is not None]
    gb = _num(base["G"]) if ref else (min(gs) if gs else None)
    fu = None
    if per_fu == "auto":
        fu = math.gcd(*base["counts"].values()) if base["counts"] else None
    elif per_fu:
        fu = float(per_fu)
    extra = []
    if fu:
        extra.append(("fu", fu))
    if per_site:
        extra.append(("site", float(per_site)))
    fields = RANK_BASE + ["%s_per_%s_%s" % (q, n, u) for n, _ in extra
                          for q in ("dE", "dG") for u in ("kJ", "eV")]
    fields += [d.split("=", 1)[0] for d in descriptors] + ["excluded", "flags"]
    for r in recs:
        r["dE"] = float(r["E"]) - e0 if r in usable else None
        g = _num(r["G"])
        r["dG"] = g - gb if r in usable and g is not None and gb is not None else None
    if sort == "G" and any(r["dG"] is not None for r in recs):
        k = "dG"
    else:
        k = "dE"
    recs.sort(key=lambda r: (r[k] is None, r[k] if r[k] is not None else 0))
    rows = []
    f = lambda v, s=1.0: "-" if v is None else "%.4f" % (v * s)
    for r in recs:
        row = {"name": r["name"], "composition": _hill(r["counts"]),
               "cell": "-" if r["lattice"] is None else
               " ".join("%.3f" % x for v in r["lattice"] for x in v),
               "code": r["code"], "method": r["method"], "solvent": r["solvent"],
               "E_el": r["E"], "G": r["G"],
               "dE_kJ": f(r["dE"], KJ_PER_EH), "dE_eV": f(r["dE"], EV_PER_EH),
               "dG_kJ": f(r["dG"], KJ_PER_EH), "dG_eV": f(r["dG"], EV_PER_EH),
               "excluded": r["excluded"], "flags": "; ".join(r["flags"])}
        for n, div in extra:
            for q in ("dE", "dG"):
                v = None if r[q] is None else r[q] / div
                row["%s_per_%s_kJ" % (q, n)] = f(v, KJ_PER_EH)
                row["%s_per_%s_eV" % (q, n)] = f(v, EV_PER_EH)
        for d in descriptors:
            name, _, expr = d.partition("=")
            row[name] = descriptor_value(r, expr)
        rows.append(row)
        if r["flags"] and not r["excluded"]:
            warns.append("%s: %s" % (r["name"], "; ".join(r["flags"])))
    return fields, rows, warns


def rank_latex(fields, rows):
    return "\n".join([r"\begin{tabular}{" + "l" * len(fields) + "}", r"\hline",
                      " & ".join(_tex(k) for k in fields) + r" \\", r"\hline"] +
                     [" & ".join(_tex(r[k]) for k in fields) + r" \\" for r in rows] +
                     [r"\hline", r"\end{tabular}"]) + "\n"


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
    ap.add_argument("--total", action="store_true",
                    help="with --checks: add a last row with the summed core-hours")
    ap.add_argument("--compare-forces", action="store_true",
                    help="compare final stored forces of exactly two calculations")
    ap.add_argument("--json", action="store_true", help="with --compare-forces: JSON output")
    ap.add_argument("--rank", choices=["csv", "latex"],
                    help="rank the calculations of a folder by energy (#703)")
    ap.add_argument("--ref", help="with --rank: the reference calculation (name)")
    ap.add_argument("--per-fu", help="with --rank: formula units per cell (N or auto)")
    ap.add_argument("--per-site", help="with --rank: substituted atoms per cell")
    ap.add_argument("--substitution", help="with --rank: A>B, B replaces A in the series")
    ap.add_argument("--treatment", help="with --rank: ecce-thermo treatment for G")
    ap.add_argument("--sort", choices=["E", "G"], default="E")
    ap.add_argument("--descriptor", action="append", default=[],
                    metavar="NAME=EXPR", help="with --rank: pattern | cn:EL | dist:EL-EL")
    ap.add_argument("--table", choices=["csv", "latex"])
    ap.add_argument("--table-file")
    ap.add_argument("--include-excluded", action="store_true",
                    help="also list calculations marked excluded (ecce:excluded)")
    a = ap.parse_args(argv)
    if a.compare_forces:
        if len(a.calcs) != 2 or a.rank or a.checks or a.table:
            ap.error("--compare-forces requires exactly two calculations and no other table mode")
        exe = os.path.join(os.environ.get("ECCE_HOME", ""), "bin", "ecce-force-compare")
        if not os.path.isfile(exe):
            exe = "ecce-force-compare"
        try:
            result = subprocess.run([exe] + a.calcs + (["--json"] if a.json else []),
                                    text=True, capture_output=True)
        except OSError as error:
            print("ecce-si-export: %s" % error, file=sys.stderr)
            return 1
        if a.output:
            open(a.output, "w").write(result.stdout)
        else:
            sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        return result.returncode
    if a.json:
        ap.error("--json requires --compare-forces")
    if a.rank:
        try:
            sub = tuple(a.substitution.split(">", 1)) if a.substitution else None
            fields, rows, warns = rank(a.calcs, a.ref, a.per_fu, a.per_site,
                                       a.treatment, a.descriptor, sub, a.sort)
        except (OSError, ValueError) as e:
            print("ecce-si-export: %s" % e, file=sys.stderr)
            return 1
        for w in warns:
            print("warning: " + w, file=sys.stderr)
        if not rows:
            return 1
        text = csv_text(rows, fields) if a.rank == "csv" else rank_latex(fields, rows)
        if a.output:
            open(a.output, "w").write(text)
        else:
            sys.stdout.write(text)
        return 0
    if a.checks:
        try:
            recs = [read_checks(c) for c in expand_calcs(a.calcs)]
            if a.total:
                recs.append(total_row(recs))
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
